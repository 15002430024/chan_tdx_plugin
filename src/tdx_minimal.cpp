// ============================================================================
// 缠论通达信DLL插件 - 标准接口版本 v4
// 参考：louis-gg/tdxchanbi 和 rust-chan 的标准写法
// 
// 核心要点：
// 1. PluginTCalcFuncInfo 只需要 nFuncMark + pCallFunc（共6字节）
// 2. RegisterTdxFunc 签名：BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun)
// 3. 公式调用方式：TDXDLL1(编号, H, L, C)，不是函数名
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

// ============================================================================
// 通达信标准插件接口（官方规范）
// ============================================================================

// 计算函数签名：固定4个参数
typedef void (*pPluginFUNC)(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc);

// 函数信息结构体：必须1字节对齐，只有2个字段
#pragma pack(push, 1)
typedef struct tagPluginTCalcFuncInfo {
    unsigned short nFuncMark;   // 函数编号（1, 2, 3...）
    pPluginFUNC pCallFunc;      // 函数指针
} PluginTCalcFuncInfo;
#pragma pack(pop)

// ============================================================================
// 调试日志
// ============================================================================

static void WriteDebugLog(const char* msg) {
    FILE* f = fopen("D:\\chan_debug.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, msg);
        fclose(f);
    }
}

// ============================================================================
// 缠论计算函数
// 
// 参数说明：
//   DataLen - K线数量
//   pfOUT   - 输出数组
//   pfINa   - HIGH数组（或自定义输入1）
//   pfINb   - LOW数组（或自定义输入2）
//   pfINc   - CLOSE数组（或自定义输入3/模式控制）
// ============================================================================

// 函数1：分型识别
// 公式调用：FX:TDXDLL1(1, H, L, C);
// 返回值：1=顶分型, -1=底分型, 0=无
void FenXing(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("FenXing 被调用!");
    
    if (pfOUT == NULL || DataLen < 3) return;
    
    // 初始化输出
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0.0f;
    }
    
    // 分型识别（简化版，未做K线包含处理）
    for (int i = 1; i < DataLen - 1; i++) {
        float h_prev = pfINa[i-1];
        float h_curr = pfINa[i];
        float h_next = pfINa[i+1];
        float l_prev = pfINb[i-1];
        float l_curr = pfINb[i];
        float l_next = pfINb[i+1];
        
        // 顶分型：中间K线的高点最高且低点最高
        if (h_curr > h_prev && h_curr > h_next && 
            l_curr > l_prev && l_curr > l_next) {
            pfOUT[i] = 1.0f;
        }
        // 底分型：中间K线的低点最低且高点最低
        else if (l_curr < l_prev && l_curr < l_next &&
                 h_curr < h_prev && h_curr < h_next) {
            pfOUT[i] = -1.0f;
        }
    }
    
    char buf[128];
    sprintf(buf, "FenXing 完成, DataLen=%d", DataLen);
    WriteDebugLog(buf);
}

// 函数2：笔端点（预留）
// 公式调用：BI:TDXDLL1(2, H, L, C);
void BiDuanDian(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("BiDuanDian 被调用!");
    
    if (pfOUT == NULL) return;
    
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0.0f;
    }
    
    // TODO: 实现笔端点识别逻辑
}

// 函数3：中枢高点（预留）
// 公式调用：ZS_H:TDXDLL1(3, H, L, C);
void ZhongShuGao(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("ZhongShuGao 被调用!");
    
    if (pfOUT == NULL) return;
    
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0.0f;
    }
    
    // TODO: 实现中枢高点识别逻辑
}

// 函数4：中枢低点（预留）
// 公式调用：ZS_L:TDXDLL1(4, H, L, C);
void ZhongShuDi(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("ZhongShuDi 被调用!");
    
    if (pfOUT == NULL) return;
    
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0.0f;
    }
    
    // TODO: 实现中枢低点识别逻辑
}

// 函数5：买点信号（预留）
// 公式调用：BUY:TDXDLL1(5, H, L, C);
void MaiDian(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("MaiDian 被调用!");
    
    if (pfOUT == NULL) return;
    
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0.0f;
    }
    
    // TODO: 实现买点识别逻辑
}

// 函数6：卖点信号（预留）
// 公式调用：SELL:TDXDLL1(6, H, L, C);
void MaiChu(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("MaiChu 被调用!");
    
    if (pfOUT == NULL) return;
    
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0.0f;
    }
    
    // TODO: 实现卖点识别逻辑
}

// 函数7：测试函数（返回K线序号，用于验证DLL是否正常工作）
// 公式调用：TEST:TDXDLL1(7, H, L, C);
void TestFunc(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    WriteDebugLog("TestFunc 被调用!");
    
    if (pfOUT == NULL) return;
    
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = (float)i;  // 返回K线序号，用于验证
    }
}

// ============================================================================
// 函数注册数组 - 必须是全局变量，必须以 {0, NULL} 结尾
// ============================================================================

PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1, (pPluginFUNC)&FenXing},       // 编号1: 分型
    {2, (pPluginFUNC)&BiDuanDian},    // 编号2: 笔端点
    {3, (pPluginFUNC)&ZhongShuGao},   // 编号3: 中枢高
    {4, (pPluginFUNC)&ZhongShuDi},    // 编号4: 中枢低
    {5, (pPluginFUNC)&MaiDian},       // 编号5: 买点
    {6, (pPluginFUNC)&MaiChu},        // 编号6: 卖点
    {7, (pPluginFUNC)&TestFunc},      // 编号7: 测试
    {0, NULL}  // ← 必须以 {0, NULL} 结尾！
};

// ============================================================================
// 导出函数 - 通达信标准接口
// ============================================================================

extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    char buf[256];
    sprintf(buf, "RegisterTdxFunc 被调用! sizeof(PluginTCalcFuncInfo)=%d, pFun=%p", 
            (int)sizeof(PluginTCalcFuncInfo), pFun);
    WriteDebugLog(buf);
    
    if (pFun == NULL) {
        WriteDebugLog("错误: pFun 为 NULL");
        return FALSE;
    }
    
    if (*pFun == NULL) {
        *pFun = g_CalcFuncSets;
        
        sprintf(buf, "函数数组已注册: g_CalcFuncSets=%p, 第一个函数编号=%d", 
                g_CalcFuncSets, g_CalcFuncSets[0].nFuncMark);
        WriteDebugLog(buf);
        
        return TRUE;
    }
    
    WriteDebugLog("*pFun 不为 NULL，跳过注册");
    return FALSE;
}

// ============================================================================
// DLL入口点
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        WriteDebugLog("=== chan.dll DLL_PROCESS_ATTACH (v4 标准接口) ===");
        break;
    case DLL_PROCESS_DETACH:
        WriteDebugLog("=== chan.dll DLL_PROCESS_DETACH ===");
        break;
    }
    return TRUE;
}
