// ============================================================================
// 缠论通达信DLL插件 - 通达信接口实现
// ============================================================================

#include "tdx_interface.h"
#include "chan_core.h"
#include "config_reader.h"
#include "logger.h"
#include <cstring>
#include <cmath>
#include <memory>
#include <chrono>

// ============================================================================
// 函数信息定义
// ============================================================================

// 注册的函数数量 (阶段五扩展: 24个函数)
#define FUNC_COUNT 24

// 函数信息数组
static PluginTCalcFuncInfo g_FuncInfo[FUNC_COUNT];

// 全局缠论核心实例（用于缓存计算结果）
static std::unique_ptr<chan::ChanCore> g_ChanCore;
static int g_LastCount = 0;  // 上次计算的K线数量
static bool g_ConfigLoaded = false;  // 配置是否已加载

// 性能统计
static long long g_TotalCalcTimeUs = 0;  // 总计算时间(微秒)
static int g_CalcCount = 0;  // 计算次数

// 初始化核心实例
static void EnsureChanCore() {
    // 首次加载配置
    if (!g_ConfigLoaded) {
        chan::InitGlobalConfig();
        g_ConfigLoaded = true;
    }
    
    if (!g_ChanCore) {
        g_ChanCore = std::make_unique<chan::ChanCore>();
        
        // 从INI配置初始化
        const auto& reader = chan::GetGlobalConfigReader();
        if (reader.IsLoaded()) {
            g_ChanCore->SetConfig(reader.ToChanConfig());
        } else {
            chan::ChanConfig config;
            config.min_bi_len = 5;
            config.min_fx_distance = 1;
            config.min_zs_bi_count = 3;
            g_ChanCore->SetConfig(config);
        }
    }
}

// 增量计算检查 - 如果K线数量变化则需要重新计算
static bool NeedRecalculate(int nCount) {
    const auto& config = chan::GetGlobalConfigReader().GetConfig();
    if (!config.enable_incremental) {
        return true;  // 禁用增量计算，总是重新计算
    }
    
    if (nCount != g_LastCount) {
        g_LastCount = nCount;
        return true;
    }
    return false;
}

// ============================================================================
// 辅助函数
// ============================================================================

// 初始化单个函数信息
static void InitFuncInfo(
    PluginTCalcFuncInfo* pInfo,
    const char* name,
    int paramCount,
    PluginTCalcFunc calcFunc,
    const char* param1Name = nullptr, float param1Min = 0, float param1Max = 100, float param1Def = 0,
    const char* param2Name = nullptr, float param2Min = 0, float param2Max = 100, float param2Def = 0
) {
    memset(pInfo, 0, sizeof(PluginTCalcFuncInfo));
    
    pInfo->nFuncMark = 0x0001;  // 固定标识
    strncpy_s(pInfo->sName, sizeof(pInfo->sName), name, _TRUNCATE);
    pInfo->nParamCount = static_cast<BYTE>(paramCount);
    pInfo->pCalcFunc = calcFunc;
    
    // 设置参数类型（全部为数值类型）
    for (int i = 0; i < 8; i++) {
        pInfo->nParamType[i] = 0;  // 0 = 数值类型
    }
    
    // 设置参数1
    if (paramCount >= 1 && param1Name) {
        strncpy_s(pInfo->sParamName[0], sizeof(pInfo->sParamName[0]), param1Name, _TRUNCATE);
        pInfo->fParamMin[0] = param1Min;
        pInfo->fParamMax[0] = param1Max;
        pInfo->fParamDef[0] = param1Def;
    }
    
    // 设置参数2
    if (paramCount >= 2 && param2Name) {
        strncpy_s(pInfo->sParamName[1], sizeof(pInfo->sParamName[1]), param2Name, _TRUNCATE);
        pInfo->fParamMin[1] = param2Min;
        pInfo->fParamMax[1] = param2Max;
        pInfo->fParamDef[1] = param2Def;
    }
}

// ============================================================================
// 导出函数实现
// ============================================================================

extern "C" __declspec(dllexport) void __stdcall RegisterTdxFunc(PluginTCalcFuncInfo** ppInfo, int* pCount)
{
    CHAN_LOG_INFO("RegisterTdxFunc 被调用");
    
    if (ppInfo == nullptr || pCount == nullptr) {
        CHAN_LOG_ERROR("RegisterTdxFunc: 参数为空");
        return;
    }
    
    // 初始化函数信息数组
    int idx = 0;
    
    // 1. CHAN_FX - 分型标记
    // 输出: 1=顶分型, -1=底分型, 0=无
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_FX", 1, CHAN_FX_Calc,
                 "N", 1, 10, 4);  // N=笔最小K线数
    
    // 2. CHAN_BI - 笔端点
    // 输出: 笔端点价格，非端点为0
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_BI", 1, CHAN_BI_Calc,
                 "N", 1, 10, 4);
    
    // 3. CHAN_DUAN - 线段端点
    // 输出: 线段端点价格
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_DUAN", 1, CHAN_DUAN_Calc,
                 "N", 1, 10, 4);
    
    // 4. CHAN_ZS_H - 中枢高点
    // 输出: 中枢区间内为ZG值
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_ZS_H", 1, CHAN_ZS_H_Calc,
                 "N", 1, 10, 4);
    
    // 5. CHAN_ZS_L - 中枢低点
    // 输出: 中枢区间内为ZD值
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_ZS_L", 1, CHAN_ZS_L_Calc,
                 "N", 1, 10, 4);
    
    // 6. CHAN_BUY - 买点信号
    // 输出: 1/2/3=买点类型, 0=无
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_BUY", 1, CHAN_BUY_Calc,
                 "N", 1, 10, 4);
    
    // 7. CHAN_SELL - 卖点信号
    // 输出: 1/2/3=卖点类型, 0=无
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_SELL", 1, CHAN_SELL_Calc,
                 "N", 1, 10, 4);
    
    // 8. CHAN_BC - 背驰标记
    // 输出: 1=顶背驰, -1=底背驰, 0=无
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_BC", 1, CHAN_BC_Calc,
                 "N", 1, 10, 4);
    
    // 9. CHAN_DIR - 方向判断 (阶段二)
    // 输出: 1=下跌趋势后(看涨), -1=上涨趋势后(看跌), 0=震荡
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_DIR", 1, CHAN_DIR_Calc,
                 "N", 1, 10, 4);
    
    // 10. CHAN_GG - 顶点价格序列 (阶段二)
    // 输出: GGn价格
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_GG", 2, CHAN_GG_Calc,
                 "N", 1, 10, 4,     // N=笔最小K线数
                 "IDX", 1, 5, 1);   // IDX=GG序号(1-5)
    
    // 11. CHAN_DD - 底点价格序列 (阶段二)
    // 输出: DDn价格
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_DD", 2, CHAN_DD_Calc,
                 "N", 1, 10, 4,
                 "IDX", 1, 5, 1);
    
    // 12. CHAN_HH - 顶点距离序列 (阶段二)
    // 输出: 到最近第n个顶点的K线数
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_HH", 2, CHAN_HH_Calc,
                 "N", 1, 10, 4,
                 "IDX", 1, 5, 1);
    
    // 13. CHAN_LL - 底点距离序列 (阶段二)
    // 输出: 到最近第n个底点的K线数
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_LL", 2, CHAN_LL_Calc,
                 "N", 1, 10, 4,
                 "IDX", 1, 5, 1);
    
    // 14. CHAN_AMP - 幅度检查 (阶段二)
    // 输出: 满足幅度条件返回1
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_AMP", 2, CHAN_AMP_Calc,
                 "N", 1, 10, 4,
                 "TYPE", 1, 3, 1); // TYPE: 1=KJA, 2=KJB, 3=二买幅度
    
    // 15. CHAN_BUY - 买点信号 (阶段三)
    // 输出: 买点类型 (11=一买A, 12=一买B, 13=一买AAA, 21=二买A, 22=二买B1, 23=二买B2, 31=三买A)
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_BUY", 2, CHAN_BUY_Calc,
                 "N", 1, 10, 4,     // N=笔最小K线数
                 "TYPE", 0, 3, 0);  // TYPE: 0=全部, 1=一买, 2=二买, 3=三买
    
    // 16. CHAN_SELL - 卖点信号 (阶段三)
    // 输出: 卖点类型 (11=一卖A, 12=一卖B, 13=一卖AAA, 21=二卖A, 22=二卖B1, 23=二卖B2, 31=三卖A)
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_SELL", 2, CHAN_SELL_Calc,
                 "N", 1, 10, 4,
                 "TYPE", 0, 3, 0);
    
    // 17. CHAN_BUYX - 综合买点信号 (阶段四)
    // 输出: 综合买点类型 (1=一买, 2=二买, 3=三买, 11=准一买, 12=准二买, 13=准三买, 21=类二买)
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_BUYX", 1, CHAN_BUYX_Calc,
                 "N", 1, 10, 4);
    
    // 18. CHAN_SELLX - 综合卖点信号 (阶段四)
    // 输出: 综合卖点类型 (负值，与买点对称)
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_SELLX", 1, CHAN_SELLX_Calc,
                 "N", 1, 10, 4);
    
    // ========================================================================
    // 阶段五新增函数 (19-24)
    // ========================================================================
    
    // 19. CHAN_ZS_Z - 中枢中轴
    // 输出: 中枢中轴价格 (ZG+ZD)/2
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_ZS_Z", 1, CHAN_ZS_Z_Calc,
                 "N", 1, 10, 4);
    
    // 20. CHAN_PREBUY - 准买点信号
    // 输出: 11=准一买, 12=准二买, 13=准三买
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_PREBUY", 1, CHAN_PREBUY_Calc,
                 "N", 1, 10, 4);
    
    // 21. CHAN_PRESELL - 准卖点信号
    // 输出: -11=准一卖, -12=准二卖, -13=准三卖
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_PRESELL", 1, CHAN_PRESELL_Calc,
                 "N", 1, 10, 4);
    
    // 22. CHAN_LIKE2B - 类二买信号
    // 输出: 21=类二买A, 22=类二买AAA
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_LIKE2B", 1, CHAN_LIKE2B_Calc,
                 "N", 1, 10, 4);
    
    // 23. CHAN_LIKE2S - 类二卖信号
    // 输出: -21=类二卖A, -22=类二卖AAA
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_LIKE2S", 1, CHAN_LIKE2S_Calc,
                 "N", 1, 10, 4);
    
    // 24. CHAN_NEWBAR - 去包含后的K线标记
    // 输出: 1=新K线（非合并），0=被合并K线
    InitFuncInfo(&g_FuncInfo[idx++], "CHAN_NEWBAR", 1, CHAN_NEWBAR_Calc,
                 "N", 1, 10, 4);
    
    // 返回函数信息数组指针和数量
    *ppInfo = g_FuncInfo;
    *pCount = idx;
    
    CHAN_LOG_INFO("已注册 %d 个函数", idx);
}

// ============================================================================
// 计算函数实现 (P1阶段: 占位实现，输出全0)
// ============================================================================

// 分型标记函数
void __stdcall CHAN_FX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_FX_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    // 获取参数（笔最小K线数）
    int minBiLen = 5;
    if (pParam && pParam[0] >= 1 && pParam[0] <= 10) {
        minBiLen = static_cast<int>(pParam[0]);
    }
    
    // 更新配置
    chan::ChanConfig config = g_ChanCore->GetConfig();
    config.min_bi_len = minBiLen;
    g_ChanCore->SetConfig(config);
    
    // 执行分析
    g_ChanCore->Analyze(pHigh, pLow, pClose, pVol, nCount);
    g_LastCount = nCount;
    
    // 输出分型标记
    g_ChanCore->OutputFX(pOut, nCount);
}

// 笔端点函数
void __stdcall CHAN_BI_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_BI_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    // 获取参数
    int minBiLen = 5;
    if (pParam && pParam[0] >= 1 && pParam[0] <= 10) {
        minBiLen = static_cast<int>(pParam[0]);
    }
    
    // 更新配置
    chan::ChanConfig config = g_ChanCore->GetConfig();
    config.min_bi_len = minBiLen;
    g_ChanCore->SetConfig(config);
    
    // 如果K线数量变化，重新计算
    if (nCount != g_LastCount) {
        g_ChanCore->Analyze(pHigh, pLow, pClose, pVol, nCount);
        g_LastCount = nCount;
    }
    
    // 输出笔端点
    g_ChanCore->OutputBI(pOut, nCount);
}

// 线段端点函数
void __stdcall CHAN_DUAN_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_DUAN_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    // 线段识别暂未实现，输出全0
    memset(pOut, 0, nCount * sizeof(float));
    
    // TODO: 实现线段划分逻辑（阶段二）
}

// 中枢高点函数
void __stdcall CHAN_ZS_H_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_ZS_H_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    // 如果K线数量变化，重新计算
    if (nCount != g_LastCount) {
        int minBiLen = 5;
        if (pParam && pParam[0] >= 1 && pParam[0] <= 10) {
            minBiLen = static_cast<int>(pParam[0]);
        }
        chan::ChanConfig config = g_ChanCore->GetConfig();
        config.min_bi_len = minBiLen;
        g_ChanCore->SetConfig(config);
        g_ChanCore->Analyze(pHigh, pLow, pClose, pVol, nCount);
        g_LastCount = nCount;
    }
    
    // 输出中枢高点
    g_ChanCore->OutputZS_H(pOut, nCount);
}

// 中枢低点函数
void __stdcall CHAN_ZS_L_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_ZS_L_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    // 如果K线数量变化，重新计算
    if (nCount != g_LastCount) {
        int minBiLen = 5;
        if (pParam && pParam[0] >= 1 && pParam[0] <= 10) {
            minBiLen = static_cast<int>(pParam[0]);
        }
        chan::ChanConfig config = g_ChanCore->GetConfig();
        config.min_bi_len = minBiLen;
        g_ChanCore->SetConfig(config);
        g_ChanCore->Analyze(pHigh, pLow, pClose, pVol, nCount);
        g_LastCount = nCount;
    }
    
    // 输出中枢低点
    g_ChanCore->OutputZS_L(pOut, nCount);
}

// 买点信号函数 (阶段三)
void __stdcall CHAN_BUY_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                             float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pAmount;  // 未使用
    (void)pParam;   // 未使用
    
    CHAN_LOG_DEBUG("CHAN_BUY_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出买点信号 (传入low数组用于价格比较)
    g_ChanCore->OutputBuySignal(pOut, nCount, pLow);
}

// 卖点信号函数 (阶段三)
void __stdcall CHAN_SELL_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pAmount;  // 未使用
    (void)pParam;   // 未使用
    
    CHAN_LOG_DEBUG("CHAN_SELL_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出卖点信号 (传入high数组用于价格比较)
    g_ChanCore->OutputSellSignal(pOut, nCount, pHigh);
}

// 背驰标记函数
void __stdcall CHAN_BC_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_BC_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    // P1阶段: 输出全0
    memset(pOut, 0, nCount * sizeof(float));
    
    // TODO P6: 实现背驰判断逻辑
}

// ============================================================================
// 阶段二：递归引用系统函数实现
// ============================================================================

// 方向判断函数
void __stdcall CHAN_DIR_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                             float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_DIR_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    // 执行计算
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出方向
    g_ChanCore->OutputDirection(pOut, nCount);
}

// GG顶点价格函数
void __stdcall CHAN_GG_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_GG_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    int idx = (pParam != nullptr) ? static_cast<int>(pParam[1]) : 1;
    if (idx < 1) idx = 1;
    if (idx > 5) idx = 5;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    g_ChanCore->OutputGG(pOut, nCount, idx);
}

// DD底点价格函数
void __stdcall CHAN_DD_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_DD_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    int idx = (pParam != nullptr) ? static_cast<int>(pParam[1]) : 1;
    if (idx < 1) idx = 1;
    if (idx > 5) idx = 5;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    g_ChanCore->OutputDD(pOut, nCount, idx);
}

// HH顶点距离函数
void __stdcall CHAN_HH_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_HH_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    int idx = (pParam != nullptr) ? static_cast<int>(pParam[1]) : 1;
    if (idx < 1) idx = 1;
    if (idx > 5) idx = 5;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    g_ChanCore->OutputHH(pOut, nCount, idx);
}

// LL底点距离函数
void __stdcall CHAN_LL_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_LL_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    int idx = (pParam != nullptr) ? static_cast<int>(pParam[1]) : 1;
    if (idx < 1) idx = 1;
    if (idx > 5) idx = 5;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    g_ChanCore->OutputLL(pOut, nCount, idx);
}

// 幅度检查函数
void __stdcall CHAN_AMP_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                             float* pClose, float* pVol, float* pAmount, float* pParam)
{
    CHAN_LOG_DEBUG("CHAN_AMP_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    int type = (pParam != nullptr) ? static_cast<int>(pParam[1]) : 1;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 填充输出
    memset(pOut, 0, nCount * sizeof(float));
    
    for (int i = 0; i < nCount; ++i) {
        bool result = false;
        switch (type) {
            case 1:  // KJA
                result = g_ChanCore->CheckFirstBuyKJA(i);
                break;
            case 2:  // KJB
                result = g_ChanCore->CheckFirstBuyKJB(i);
                break;
            case 3:  // 二买幅度
                result = g_ChanCore->CheckSecondBuyAmplitude(i);
                break;
        }
        pOut[i] = result ? 1.0f : 0.0f;
    }
}

// 综合买点信号函数 (阶段四)
void __stdcall CHAN_BUYX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    CHAN_LOG_DEBUG("CHAN_BUYX_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出综合买点信号
    g_ChanCore->OutputCombinedBuySignal(pOut, nCount, pLow);
}

// 综合卖点信号函数 (阶段四)
void __stdcall CHAN_SELLX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                               float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    CHAN_LOG_DEBUG("CHAN_SELLX_Calc: nCount=%d", nCount);
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出综合卖点信号
    g_ChanCore->OutputCombinedSellSignal(pOut, nCount, pHigh);
}

// ============================================================================
// 阶段五新增函数实现
// ============================================================================

// 中枢中轴函数
void __stdcall CHAN_ZS_Z_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->CheckZS();
    
    // 输出中枢中轴
    g_ChanCore->OutputZS_Z(pOut, nCount);
}

// 准买点信号函数
void __stdcall CHAN_PREBUY_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出准买点信号
    g_ChanCore->OutputPreBuySignal(pOut, nCount, pLow);
}

// 准卖点信号函数
void __stdcall CHAN_PRESELL_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                 float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出准卖点信号
    g_ChanCore->OutputPreSellSignal(pOut, nCount, pHigh);
}

// 类二买信号函数
void __stdcall CHAN_LIKE2B_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出类二买信号
    g_ChanCore->OutputLikeSecondBuySignal(pOut, nCount, pLow);
}

// 类二卖信号函数
void __stdcall CHAN_LIKE2S_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    g_ChanCore->CheckFX();
    g_ChanCore->CheckBI();
    g_ChanCore->BuildBiSequence(nCount - 1);
    
    // 输出类二卖信号
    g_ChanCore->OutputLikeSecondSellSignal(pOut, nCount, pHigh);
}

// 去包含后新K线标记函数
void __stdcall CHAN_NEWBAR_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam)
{
    (void)pClose; (void)pVol; (void)pAmount; (void)pParam;
    
    if (pOut == nullptr || nCount <= 0) return;
    
    EnsureChanCore();
    
    g_ChanCore->RemoveInclude(pHigh, pLow, nCount);
    
    // 输出新K线标记
    g_ChanCore->OutputNewBar(pOut, nCount);
}
