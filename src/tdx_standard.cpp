// ============================================================================
// 缠论通达信DLL插件 - 完整版标准接口（纯导出映射层）
// ============================================================================
// v7.5 (2026-03-10) - 统一算法架构
// chan_core.cpp 为唯一算法真源，本文件只保留：
//   - RegisterTdxFunc + 编号映射
//   - 22 个 pfOUT 填充导出函数（只做数组遍历 + 赋值，不含判定逻辑）
//   - CalcMA()（均线计算，ChanCore 不含此逻辑）
//   - LoadConfig() 简易配置读取
//   - WriteLog() 调试日志
// ============================================================================

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <cmath>
#include <set>

#include "chan_core.h"

// ============================================================================
// 通达信标准插件接口
// ============================================================================

typedef void (*pPluginFUNC)(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc);

#pragma pack(push, 1)
typedef struct tagPluginTCalcFuncInfo {
    unsigned short nFuncMark;
    pPluginFUNC pCallFunc;
} PluginTCalcFuncInfo;
#pragma pack(pop)

// ============================================================================
// 调试日志
// ============================================================================

static FILE* g_LogFile = NULL;

static void WriteLog(const char* msg) {
    if (!g_LogFile) {
        g_LogFile = fopen("D:\\chan_debug.log", "a");
    }
    if (g_LogFile) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(g_LogFile, "[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, msg);
        fflush(g_LogFile);
    }
}

// ============================================================================
// [v7.5] 简易配置读取
// ============================================================================

static int g_MinBiLen = 5;
static int g_MinZsBiCount = 3;
static bool g_ConfigLoaded = false;

static void LoadConfig() {
    if (g_ConfigLoaded) return;
    g_ConfigLoaded = true;

    // 获取 DLL 所在目录
    char dllPath[MAX_PATH] = {0};
    HMODULE hMod = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                       (LPCSTR)&LoadConfig, &hMod);
    GetModuleFileNameA(hMod, dllPath, MAX_PATH);

    // 替换文件名为 CZSC.ini
    char* lastSlash = strrchr(dllPath, '\\');
    if (lastSlash) {
        strcpy(lastSlash + 1, "CZSC.ini");
    }

    // 键名必须与 CZSC.ini 完全一致：min_bi_length, min_zs_bi_count
    g_MinBiLen = GetPrivateProfileIntA("General", "min_bi_length", 5, dllPath);
    g_MinZsBiCount = GetPrivateProfileIntA("General", "min_zs_bi_count", 3, dllPath);

    char logMsg[256];
    sprintf(logMsg, "[v7.5] Config: min_bi_length=%d, min_zs_bi_count=%d, path=%s",
            g_MinBiLen, g_MinZsBiCount, dllPath);
    WriteLog(logMsg);
}

// ============================================================================
// [v7.5] 全局 ChanCore 实例 + 辅助缓存
// ============================================================================

static chan::ChanCore g_Core;
static std::vector<float> g_MA13;
static std::vector<float> g_MA26;
static std::vector<float> g_Closes;
static int g_LastCount = 0;
static float g_LastHigh0 = 0;
static float g_LastLow0 = 0;

// 计算简单移动平均线
static void CalcMA(const float* closes, int count, int period, std::vector<float>& ma) {
    ma.resize(count);
    for (int i = 0; i < count; ++i) {
        if (i < period - 1) {
            ma[i] = closes[i];
        } else {
            float sum = 0;
            for (int j = 0; j < period; ++j) {
                sum += closes[i - j];
            }
            ma[i] = sum / period;
        }
    }
}

// ============================================================================
// 完整分析流程（带均线计算）
// ============================================================================

static void FullAnalyzeWithMA(const float* highs, const float* lows, const float* closes, int count) {
    // [v7.5] 缓存暂时禁用（见 Step 1.1）
    // TODO: 后续恢复时至少比较 count + highs[count-1] + lows[count-1] + closes[count-1]

    g_LastCount = count;
    g_LastHigh0 = (count > 0) ? highs[0] : 0;
    g_LastLow0 = (count > 0) ? lows[0] : 0;

    // 缓存收盘价
    g_Closes.resize(count);
    for (int i = 0; i < count; ++i) {
        g_Closes[i] = closes[i];
    }

    // 计算均线（ChanCore 不含 MA 逻辑）
    CalcMA(closes, count, 13, g_MA13);
    CalcMA(closes, count, 26, g_MA26);

    // 加载配置
    LoadConfig();

    // 将配置注入 ChanCore
    chan::ChanConfig cfg = g_Core.GetConfig();
    cfg.min_bi_len = g_MinBiLen;
    cfg.min_zs_bi_count = g_MinZsBiCount;
    g_Core.SetConfig(cfg);

    // 一次性完整分析（去包含 + 分型 + 笔 + 中枢）
    g_Core.Analyze(highs, lows, closes, nullptr, count);

    // 设置均线数据（用于买卖点判断）
    g_Core.SetMAData(g_MA13.data(), g_MA26.data(), count);

    // 构建递归引用序列（GG/DD/HH/LL + 方向）
    if (count > 0) {
        g_Core.BuildBiSequence(count - 1);
    }
}

// ============================================================================
// 通达信导出函数（只做数组遍历 + 赋值，不含任何判定逻辑）
// ============================================================================

// 函数1：分型识别
// 公式调用：FX:TDXDLL1(1, H, L, C);
// 返回值：1=顶分型, -1=底分型, 0=无
void FenXing(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (const auto& fx : g_Core.GetFractals()) {
        int idx = fx.kline_idx;
        if (idx >= 0 && idx < DataLen) {
            pfOUT[idx] = (fx.type == chan::FractalType::TOP) ? 1.0f : -1.0f;
        }
    }
}

// 函数2：笔端点类型
// 公式调用：BI:TDXDLL1(2, H, L, C);
// 返回值：1=顶端点, -1=底端点, 0=非端点
void BiDuanDian(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (const auto& bi : g_Core.GetStrokes()) {
        if (bi.direction == chan::Direction::UP) {
            // 向上笔：起点是底，终点是顶
            if (bi.start_idx >= 0 && bi.start_idx < DataLen)
                pfOUT[bi.start_idx] = -1.0f;
            if (bi.end_idx >= 0 && bi.end_idx < DataLen)
                pfOUT[bi.end_idx] = 1.0f;
        } else if (bi.direction == chan::Direction::DOWN) {
            // 向下笔：起点是顶，终点是底
            if (bi.start_idx >= 0 && bi.start_idx < DataLen)
                pfOUT[bi.start_idx] = 1.0f;
            if (bi.end_idx >= 0 && bi.end_idx < DataLen)
                pfOUT[bi.end_idx] = -1.0f;
        }
    }
}

// 函数3：中枢高点
// 公式调用：ZS_H:TDXDLL1(3, H, L, C);
void ZhongShuGao(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    const auto& pivots = g_Core.GetPivots();
    size_t pivotCount = pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const auto& zs = pivots[p];
        int actual_end = zs.end_idx;
        // 边界收缩：如果当前中枢end与下一个中枢start重叠，收缩1格避免覆盖
        if (p + 1 < pivotCount && zs.end_idx >= pivots[p + 1].start_idx) {
            actual_end = pivots[p + 1].start_idx - 1;
        }
        for (int i = zs.start_idx; i <= actual_end && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = zs.ZG;
            }
        }
    }
}

// 函数4：中枢低点
// 公式调用：ZS_L:TDXDLL1(4, H, L, C);
void ZhongShuDi(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    const auto& pivots = g_Core.GetPivots();
    size_t pivotCount = pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const auto& zs = pivots[p];
        int actual_end = zs.end_idx;
        if (p + 1 < pivotCount && zs.end_idx >= pivots[p + 1].start_idx) {
            actual_end = pivots[p + 1].start_idx - 1;
        }
        for (int i = zs.start_idx; i <= actual_end && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = zs.ZD;
            }
        }
    }
}

// 函数5：中枢中轴
// 公式调用：ZS_Z:TDXDLL1(5, H, L, C);
void ZhongShuZhong(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    const auto& pivots = g_Core.GetPivots();
    size_t pivotCount = pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const auto& zs = pivots[p];
        int actual_end = zs.end_idx;
        if (p + 1 < pivotCount && zs.end_idx >= pivots[p + 1].start_idx) {
            actual_end = pivots[p + 1].start_idx - 1;
        }
        for (int i = zs.start_idx; i <= actual_end && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = zs.ZZ;
            }
        }
    }
}

// 函数6：笔方向
// 公式调用：BI_DIR:TDXDLL1(6, H, L, C);
// 返回值：1=向上笔, -1=向下笔, 0=不在笔内
void BiDirection(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (const auto& bi : g_Core.GetStrokes()) {
        float dir = static_cast<float>(bi.direction);
        for (int i = bi.start_idx; i <= bi.end_idx && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = dir;
            }
        }
    }
}

// 函数7：买点信号（完整实现速查手册逻辑）
// 公式调用：BUY:TDXDLL1(7, H, L, C);
// 返回值：1/2=一买A/B, 11/12/13=二买A/B1/B2, 21=三买, 31/32/33=准买点, 0=无
void BuySignal(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    // 遍历所有向下笔的终点，检查买点条件
    for (const auto& bi : g_Core.GetStrokes()) {
        if (!bi.is_confirmed) continue;
        if (bi.direction != chan::Direction::DOWN) continue;

        int idx = bi.end_idx;
        if (idx < 0 || idx >= DataLen) continue;

        float low = pfINb[idx];

        // 一买检查
        chan::FirstBuyType fb = g_Core.CheckFirstBuy(idx, low);
        if (fb != chan::FirstBuyType::NONE) {
            // TYPE_A=1, TYPE_B=2, TYPE_AAA→1（与v7.4信号编码兼容）
            int val = static_cast<int>(fb);
            if (val == 3) val = 1;  // AAA 归入 A
            pfOUT[idx] = (float)val;
            continue;
        }

        // 二买检查
        chan::SecondBuyType sb = g_Core.CheckSecondBuy(idx, low);
        if (sb != chan::SecondBuyType::NONE) {
            pfOUT[idx] = 10.0f + static_cast<float>(sb);  // 11, 12, 13
            continue;
        }

        // 三买检查
        chan::ThirdBuyType tb = g_Core.CheckThirdBuy(idx, low);
        if (tb != chan::ThirdBuyType::NONE) {
            pfOUT[idx] = 20.0f + static_cast<float>(tb);  // 21
            continue;
        }

        // 准买点检查（优先级最低）
        chan::PreFirstBuyType pfb = g_Core.CheckPreFirstBuy(idx, low);
        if (pfb != chan::PreFirstBuyType::NONE) {
            pfOUT[idx] = 31.0f;
            continue;
        }

        chan::PreSecondBuyType psb = g_Core.CheckPreSecondBuy(idx, low);
        if (psb != chan::PreSecondBuyType::NONE) {
            pfOUT[idx] = 32.0f;
            continue;
        }

        chan::PreThirdBuyType ptb = g_Core.CheckPreThirdBuy(idx, low);
        if (ptb != chan::PreThirdBuyType::NONE) {
            pfOUT[idx] = 33.0f;
            continue;
        }
    }
}

// 函数8：卖点信号（完整实现速查手册逻辑）
// 公式调用：SELL:TDXDLL1(8, H, L, C);
// 返回值：-1/-2/-3=一卖A/B/C, -11/-12/-13=二卖, -21=三卖, -31/-32/-33=准卖点, 0=无
void SellSignal(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    // 遍历所有向上笔的终点，检查卖点条件
    for (const auto& bi : g_Core.GetStrokes()) {
        if (!bi.is_confirmed) continue;
        if (bi.direction != chan::Direction::UP) continue;

        int idx = bi.end_idx;
        if (idx < 0 || idx >= DataLen) continue;

        float high = pfINa[idx];

        // 一卖检查
        chan::FirstSellType fs = g_Core.CheckFirstSell(idx, high);
        if (fs != chan::FirstSellType::NONE) {
            int val = static_cast<int>(fs);
            // TYPE_A=1→-1, TYPE_B=2→-2, TYPE_AAA=3→-1（归入A）, TYPE_C=4→-3
            if (val == 3) val = 1;      // AAA → A
            else if (val == 4) val = 3; // TYPE_C → -3
            pfOUT[idx] = -(float)val;
            continue;
        }

        // 二卖检查
        chan::SecondSellType ss = g_Core.CheckSecondSell(idx, high);
        if (ss != chan::SecondSellType::NONE) {
            pfOUT[idx] = -10.0f - static_cast<float>(ss);  // -11, -12, -13
            continue;
        }

        // 三卖检查
        chan::ThirdSellType ts = g_Core.CheckThirdSell(idx, high);
        if (ts != chan::ThirdSellType::NONE) {
            pfOUT[idx] = -20.0f - static_cast<float>(ts);  // -21
            continue;
        }

        // 准卖点检查
        chan::PreFirstSellType pfs = g_Core.CheckPreFirstSell(idx, high);
        if (pfs != chan::PreFirstSellType::NONE) {
            pfOUT[idx] = -31.0f;
            continue;
        }

        chan::PreSecondSellType pss = g_Core.CheckPreSecondSell(idx, high);
        if (pss != chan::PreSecondSellType::NONE) {
            pfOUT[idx] = -32.0f;
            continue;
        }

        chan::PreThirdSellType pts = g_Core.CheckPreThirdSell(idx, high);
        if (pts != chan::PreThirdSellType::NONE) {
            pfOUT[idx] = -33.0f;
            continue;
        }
    }
}

// 函数9：新K线标记（去包含后）
// 公式调用：NEWBAR:TDXDLL1(9, H, L, C);
// 返回值：1=保留的K线, 0=被合并的K线
void NewBar(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (const auto& mk : g_Core.GetMergedKLines()) {
        if (mk.merge_end >= 0 && mk.merge_end < DataLen) {
            pfOUT[mk.merge_end] = 1.0f;
        }
    }
}

// 函数10：测试函数
// 公式调用：TEST:TDXDLL1(10, H, L, C);
void TestFunc(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT) return;

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = (float)i;
    }
}

// 函数11：方向判断
// 公式调用：DIR:TDXDLL1(11, H, L, C);
// 返回值：1=下跌后(适合找买点), -1=上涨后(适合找卖点), 0=无
void Direction(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = (float)g_Core.GetDirection(i);
    }
}

// 函数12：GG1 - 最近顶点价格
// 公式调用：GG1:TDXDLL1(12, H, L, C);
void OutputGG1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = g_Core.GetGG(i, 1);
    }
}

// 函数13：DD1 - 最近底点价格
// 公式调用：DD1:TDXDLL1(13, H, L, C);
void OutputDD1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = g_Core.GetDD(i, 1);
    }
}

// 函数14：LL1 - 最近底点距当前K线数
// 公式调用：LL1:TDXDLL1(14, H, L, C);
void OutputLL1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = (float)g_Core.GetLL(i, 1);
    }
}

// 函数15：HH1 - 最近顶点距当前K线数
// 公式调用：HH1:TDXDLL1(15, H, L, C);
void OutputHH1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = (float)g_Core.GetHH(i, 1);
    }
}

// 函数16：MA13均线
// 公式调用：MA13:TDXDLL1(16, H, L, C);
void OutputMA13(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = (i < (int)g_MA13.size()) ? g_MA13[i] : pfINc[i];
    }
}

// 函数17：MA26均线
// 公式调用：MA26:TDXDLL1(17, H, L, C);
void OutputMA26(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    for (int i = 0; i < DataLen; ++i) {
        pfOUT[i] = (i < (int)g_MA26.size()) ? g_MA26[i] : pfINc[i];
    }
}

// 函数18：中枢开始标记
// 公式调用：ZSKS:TDXDLL1(18, H, L, C);
// 返回值：1=下跌中枢开始, 2=上涨中枢开始, 0=非开始位置
void ZhongShuKaiShi(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (const auto& zs : g_Core.GetPivots()) {
        if (zs.start_idx >= 0 && zs.start_idx < DataLen) {
            // 1=下跌中枢(第一笔向下), 2=上涨中枢(第一笔向上)
            pfOUT[zs.start_idx] = (zs.direction == chan::Direction::DOWN) ? 1.0f : 2.0f;
        }
    }
}

// 函数19：中枢结束标记
// 公式调用：ZSJS:TDXDLL1(19, H, L, C);
// 返回值：1=下跌中枢结束, 2=上涨中枢结束, 0=非结束位置
void ZhongShuJieShu(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    const auto& pivots = g_Core.GetPivots();
    size_t pivotCount = pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const auto& zs = pivots[p];
        int actual_end = zs.end_idx;
        if (p + 1 < pivotCount && zs.end_idx >= pivots[p + 1].start_idx) {
            actual_end = pivots[p + 1].start_idx - 1;
        }
        if (actual_end >= 0 && actual_end < DataLen) {
            pfOUT[actual_end] = (zs.direction == chan::Direction::DOWN) ? 1.0f : 2.0f;
        }
    }
}

// 函数20：笔端点高点价格（只在顶点输出）
// 公式调用：KXG:TDXDLL1(20, H, L, C);
void BiGaoDian(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    // 收集所有笔端点中的顶点
    std::set<int> top_indices;
    for (const auto& bi : g_Core.GetStrokes()) {
        if (bi.direction == chan::Direction::UP) {
            top_indices.insert(bi.end_idx);
        } else if (bi.direction == chan::Direction::DOWN) {
            top_indices.insert(bi.start_idx);
        }
    }

    // 在顶点位置输出高点价格
    for (int tidx : top_indices) {
        if (tidx >= 0 && tidx < DataLen) {
            for (const auto& bi : g_Core.GetStrokes()) {
                if (bi.direction == chan::Direction::UP && bi.end_idx == tidx) {
                    pfOUT[tidx] = bi.high;
                    break;
                } else if (bi.direction == chan::Direction::DOWN && bi.start_idx == tidx) {
                    pfOUT[tidx] = bi.high;
                    break;
                }
            }
        }
    }
}

// 函数21：笔端点低点价格（只在底点输出）
// 公式调用：KXD:TDXDLL1(21, H, L, C);
void BiDiDian(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    // 收集所有笔端点中的底点
    std::set<int> bottom_indices;
    for (const auto& bi : g_Core.GetStrokes()) {
        if (bi.direction == chan::Direction::DOWN) {
            bottom_indices.insert(bi.end_idx);
        } else if (bi.direction == chan::Direction::UP) {
            bottom_indices.insert(bi.start_idx);
        }
    }

    // 在底点位置输出低点价格
    for (int bidx : bottom_indices) {
        if (bidx >= 0 && bidx < DataLen) {
            for (const auto& bi : g_Core.GetStrokes()) {
                if (bi.direction == chan::Direction::DOWN && bi.end_idx == bidx) {
                    pfOUT[bidx] = bi.low;
                    break;
                } else if (bi.direction == chan::Direction::UP && bi.start_idx == bidx) {
                    pfOUT[bidx] = bi.low;
                    break;
                }
            }
        }
    }
}

// 函数22：中枢方向（持续输出）
// 公式调用：ZS_DIR:TDXDLL1(22, H, L, C);
// 返回值：1=上涨中枢(第一笔向上), -1=下跌中枢(第一笔向下), 0=不在中枢内
void ZhongShuFangXiang(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;

    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);

    memset(pfOUT, 0, DataLen * sizeof(float));

    for (const auto& zs : g_Core.GetPivots()) {
        float dir = static_cast<float>(zs.direction);
        for (int i = zs.start_idx; i <= zs.end_idx && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = dir;
            }
        }
    }
}

// ============================================================================
// 函数注册数组
// ============================================================================

PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1,  (pPluginFUNC)&FenXing},
    {2,  (pPluginFUNC)&BiDuanDian},
    {3,  (pPluginFUNC)&ZhongShuGao},
    {4,  (pPluginFUNC)&ZhongShuDi},
    {5,  (pPluginFUNC)&ZhongShuZhong},
    {6,  (pPluginFUNC)&BiDirection},
    {7,  (pPluginFUNC)&BuySignal},
    {8,  (pPluginFUNC)&SellSignal},
    {9,  (pPluginFUNC)&NewBar},
    {10, (pPluginFUNC)&TestFunc},
    {11, (pPluginFUNC)&Direction},
    {12, (pPluginFUNC)&OutputGG1},
    {13, (pPluginFUNC)&OutputDD1},
    {14, (pPluginFUNC)&OutputLL1},
    {15, (pPluginFUNC)&OutputHH1},
    {16, (pPluginFUNC)&OutputMA13},
    {17, (pPluginFUNC)&OutputMA26},
    {18, (pPluginFUNC)&ZhongShuKaiShi},
    {19, (pPluginFUNC)&ZhongShuJieShu},
    {20, (pPluginFUNC)&BiGaoDian},
    {21, (pPluginFUNC)&BiDiDian},
    {22, (pPluginFUNC)&ZhongShuFangXiang},
    {0,  NULL}
};

// ============================================================================
// 导出函数
// ============================================================================

extern "C" __declspec(dllexport)
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    WriteLog("RegisterTdxFunc v7.5 - 统一算法架构");

    if (pFun == NULL) {
        WriteLog("错误: pFun 为 NULL");
        return FALSE;
    }

    if (*pFun == NULL) {
        *pFun = g_CalcFuncSets;
        WriteLog("函数数组已注册: 22个函数");
        return TRUE;
    }

    return FALSE;
}

// ============================================================================
// DLL入口
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    (void)lpReserved;
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        WriteLog("=== chan.dll v7.5 统一算法架构 加载 ===");
        break;
    case DLL_PROCESS_DETACH:
        if (g_LogFile) {
            fclose(g_LogFile);
            g_LogFile = NULL;
        }
        break;
    }
    return TRUE;
}
