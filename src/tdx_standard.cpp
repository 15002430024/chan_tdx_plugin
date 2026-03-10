// ============================================================================
// 缠论通达信DLL插件 - 完整版标准接口
// ============================================================================
// 集成 ChanCore 完整算法，使用通达信标准DLL接口
// 版本: v6.0 (2026-02-01) - 完整实现速查手册所有买卖点逻辑
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
// 缠论核心数据结构
// ============================================================================

struct MergedKLine {
    int index;          // 原始索引
    float high;         // 最高价
    float low;          // 最低价
    bool is_merged;     // 是否合并
    int merge_start;    // 合并起始索引
    int merge_end;      // 合并结束索引
    int raw_high_idx;   // 高点来源的原始K线索引（REQ-001新增）
    int raw_low_idx;    // 低点来源的原始K线索引（REQ-001新增）
};

struct Fractal {
    int type;           // 1=顶分型, -1=底分型
    int index;          // K线索引（原始K线，用于绘图）
    int merged_index;   // 合并K线索引（REQ-002新增，用于逻辑计算）
    float high;         // 分型高点
    float low;          // 分型低点
};

// 重要：禁止用 memset(&stroke, 0, sizeof(stroke)) 初始化 Stroke
// memset 会将 is_confirmed 置为 false。创建 Stroke 时必须显式设置所有字段。
struct Stroke {
    int start_idx;      // 起点K线索引
    int end_idx;        // 终点K线索引
    float start_price;  // 起点价格
    float end_price;    // 终点价格
    int direction;      // 1=向上, -1=向下
    bool is_confirmed;  // true=已确认, false=未完成笔
};

struct Pivot {
    int start_idx;      // 起始K线索引
    int end_idx;        // 结束K线索引
    float zg;           // 中枢高点
    float zd;           // 中枢低点
    float zz;           // 中枢中轴
    int direction;      // 中枢方向：1=向上, -1=向下
};

// 递归引用数据（GG/DD序列）
struct BiSequenceData {
    float GG[6];        // GG1-GG5 (索引1-5)
    float DD[6];        // DD1-DD5
    int HH[6];          // HH1-HH5 (顶点距当前K线距离)
    int LL[6];          // LL1-LL5 (底点距当前K线距离)
    int direction;      // 方向：1=下跌后(适合找买点), -1=上涨后(适合找卖点)
};

// ============================================================================
// 全局缓存（避免重复计算）
// ============================================================================

static std::vector<MergedKLine> g_MergedKLines;
static std::vector<Fractal> g_Fractals;
static std::vector<Stroke> g_Strokes;
static std::vector<Pivot> g_Pivots;
static std::vector<int> g_RawToMerged;
static std::vector<float> g_MA13;       // 13周期均线
static std::vector<float> g_MA26;       // 26周期均线
static std::vector<float> g_Closes;     // 收盘价缓存
static int g_LastCount = 0;
static float g_LastHigh0 = 0;
static float g_LastLow0 = 0;

// ============================================================================
// 工具函数
// ============================================================================

static inline float MIN3(float a, float b, float c) {
    return std::min(std::min(a, b), c);
}

static inline float MAX3(float a, float b, float c) {
    return std::max(std::max(a, b), c);
}

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
// 缠论核心算法实现
// ============================================================================

// 判断包含关系
static bool HasIncludeRelation(float h1, float l1, float h2, float l2) {
    return (h1 >= h2 && l1 <= l2) || (h2 >= h1 && l2 <= l1);
}

// 去包含处理
static void RemoveInclude(const float* highs, const float* lows, int count) {
    g_MergedKLines.clear();
    g_RawToMerged.clear();
    g_RawToMerged.resize(count, -1);
    
    if (count <= 0) return;
    
    // 第一根K线
    MergedKLine first;
    first.index = 0;
    first.high = highs[0];
    first.low = lows[0];
    first.is_merged = false;
    first.merge_start = 0;
    first.merge_end = 0;
    first.raw_high_idx = 0;  // REQ-001: 高点来自第0根K线
    first.raw_low_idx = 0;   // REQ-001: 低点来自第0根K线
    g_MergedKLines.push_back(first);
    g_RawToMerged[0] = 0;
    
    int curr_dir = 0;  // 0=未定, 1=向上, -1=向下
    
    for (int i = 1; i < count; ++i) {
        MergedKLine& last = g_MergedKLines.back();
        
        if (HasIncludeRelation(last.high, last.low, highs[i], lows[i])) {
            // 存在包含关系，合并
            if (curr_dir == 0) {
                // 确定方向
                if (g_MergedKLines.size() >= 2) {
                    int prev_idx = (int)g_MergedKLines.size() - 2;
                    if (g_MergedKLines[prev_idx].high < last.high) {
                        curr_dir = 1;
                    } else if (g_MergedKLines[prev_idx].high > last.high) {
                        curr_dir = -1;
                    } else {
                        if (g_MergedKLines[prev_idx].low < last.low) {
                            curr_dir = 1;
                        } else if (g_MergedKLines[prev_idx].low > last.low) {
                            curr_dir = -1;
                        }
                        // else: 高低点都相等 → 保持 curr_dir 不变
                        // 如果 curr_dir 仍为 0，后续包含处理走 else 分支（向下合并）
                    }
                } else {
                    curr_dir = 1;
                }
            }
            
            if (curr_dir > 0) {
                // 向上：取高的高点和高的低点
                if (highs[i] > last.high) {
                    last.high = highs[i];
                    last.raw_high_idx = i;  // REQ-001: 更新高点来源
                }
                if (lows[i] > last.low) {
                    last.low = lows[i];
                    last.raw_low_idx = i;   // REQ-001: 更新低点来源
                }
            } else {
                // 向下：取低的高点和低的低点
                if (highs[i] < last.high) {
                    last.high = highs[i];
                    last.raw_high_idx = i;  // REQ-001: 更新高点来源
                }
                if (lows[i] < last.low) {
                    last.low = lows[i];
                    last.raw_low_idx = i;   // REQ-001: 更新低点来源
                }
            }
            last.is_merged = true;
            last.merge_end = i;
            g_RawToMerged[i] = (int)g_MergedKLines.size() - 1;
        } else {
            // 无包含关系，新增K线
            if (highs[i] > last.high) {
                curr_dir = 1;
            } else if (highs[i] < last.high) {
                curr_dir = -1;
            } else {
                if (lows[i] > last.low) {
                    curr_dir = 1;
                } else if (lows[i] < last.low) {
                    curr_dir = -1;
                }
                // else: 高低点都相等 → 不更新 curr_dir，保持上一次的值
            }
            
            MergedKLine curr;
            curr.index = i;
            curr.high = highs[i];
            curr.low = lows[i];
            curr.is_merged = false;
            curr.merge_start = i;
            curr.merge_end = i;
            curr.raw_high_idx = i;  // REQ-001: 高点来自当前K线
            curr.raw_low_idx = i;   // REQ-001: 低点来自当前K线
            g_MergedKLines.push_back(curr);
            g_RawToMerged[i] = (int)g_MergedKLines.size() - 1;
        }
    }
}

// 分型识别
static void CheckFX() {
    g_Fractals.clear();
    
    int n = (int)g_MergedKLines.size();
    if (n < 3) return;
    
    for (int i = 1; i < n - 1; ++i) {
        const MergedKLine& prev = g_MergedKLines[i - 1];
        const MergedKLine& curr = g_MergedKLines[i];
        const MergedKLine& next = g_MergedKLines[i + 1];
        
        // 顶分型：中间K线高点最高且低点也最高
        if (curr.high > prev.high && curr.high > next.high &&
            curr.low > prev.low && curr.low > next.low) {
            Fractal fx;
            fx.type = 1;
            fx.index = curr.raw_high_idx;  // REQ-001: 使用高点实际来源的K线索引
            fx.merged_index = i;           // REQ-002: 合并K线序列中的索引
            fx.high = curr.high;
            fx.low = curr.low;
            
            // 处理连续同类型分型：取极值
            if (!g_Fractals.empty() && g_Fractals.back().type == 1) {
                if (curr.high > g_Fractals.back().high) {
                    g_Fractals.back() = fx;
                }
            } else {
                g_Fractals.push_back(fx);
            }
        }
        // 底分型：中间K线低点最低且高点也最低
        else if (curr.low < prev.low && curr.low < next.low &&
                 curr.high < prev.high && curr.high < next.high) {
            Fractal fx;
            fx.type = -1;
            fx.index = curr.raw_low_idx;  // REQ-001: 使用低点实际来源的K线索引
            fx.merged_index = i;          // REQ-002: 合并K线序列中的索引
            fx.high = curr.high;
            fx.low = curr.low;
            
            // 处理连续同类型分型：取极值
            if (!g_Fractals.empty() && g_Fractals.back().type == -1) {
                if (curr.low < g_Fractals.back().low) {
                    g_Fractals.back() = fx;
                }
            } else {
                g_Fractals.push_back(fx);
            }
        }
    }
}

// 辅助函数：生成一笔并加入 g_Strokes
static void EmitStroke(const Fractal& fx1, const Fractal& fx2) {
    Stroke stroke;
    stroke.start_idx = fx1.index;
    stroke.end_idx = fx2.index;
    if (fx1.type == -1) {
        // 底→顶 = 上升笔
        stroke.direction = 1;
        stroke.start_price = fx1.low;
        stroke.end_price = fx2.high;
    } else {
        // 顶→底 = 下降笔
        stroke.direction = -1;
        stroke.start_price = fx1.high;
        stroke.end_price = fx2.low;
    }
    stroke.is_confirmed = true;  // 默认已确认
    g_Strokes.push_back(stroke);
}

// 笔识别 — 状态机实现（候选终点 + 极值追踪 + 未完成笔输出）
// 设计决策（不可更改）：
//   1. min_bi_len 计数口径：原始K线下标差（fx.index 是 merge_end）
//   2. 未完成笔仅用于画线，不参与 CheckZS 和买卖点计算
//   3. EmitStroke 中：上升笔 start_price=fx1.low, end_price=fx2.high
//                     下降笔 start_price=fx1.high, end_price=fx2.low
static void CheckBI(int min_bi_len = 5) {
    g_Strokes.clear();
    
    int n = (int)g_Fractals.size();
    if (n < 2) return;
    
    int start = 0;        // 当前笔起点分型索引（在 g_Fractals 中）
    int candidate = -1;   // 候选终点分型索引，-1 表示无候选
    
    for (int i = 1; i < n; ++i) {
        const Fractal& fx = g_Fractals[i];
        const Fractal& startFx = g_Fractals[start];
        
        // ---- 规则1：同类型分型 ----
        if (fx.type == startFx.type) {
            if (candidate == -1) {
                // 无候选终点：更新起点极值
                if ((fx.type == 1 && fx.high > startFx.high) ||
                    (fx.type == -1 && fx.low < startFx.low)) {
                    start = i;
                }
            } else {
                // 有候选终点：出现同向分型 = 反转证据 → 确认笔
                EmitStroke(g_Fractals[start], g_Fractals[candidate]);
                start = candidate;
                candidate = -1;
                --i;  // 回退，让当前分型在下一轮重新处理
            }
            continue;
        }
        
        // ---- 规则2：异类型分型 ----
        // 2a. 距离检查（原始K线下标差）
        int dist = fx.index - startFx.index;
        if (dist < min_bi_len) continue;
        
        // 2b. 价格有效性检查
        if (startFx.type == -1 && fx.high <= startFx.low) continue;
        if (startFx.type == 1  && fx.low  >= startFx.high) continue;
        
        // 2c. 更新候选终点
        if (candidate == -1) {
            candidate = i;  // 首个合法候选
        } else {
            const Fractal& candFx = g_Fractals[candidate];
            if ((fx.type == 1 && fx.high > candFx.high) ||
                (fx.type == -1 && fx.low < candFx.low)) {
                candidate = i;  // 极值延伸
            }
        }
    }
    
    // ---- 规则3：末端未完成笔 ----
    if (candidate != -1) {
        EmitStroke(g_Fractals[start], g_Fractals[candidate]);
        g_Strokes.back().is_confirmed = false;  // 标记为未确认
    }
}

// 中枢识别 — 前三笔锁定 ZG/ZD + 延伸不压缩 + 排除未完成笔
// 设计决策（不可更改）：
//   1. ZG/ZD 仅由前三笔确定，后续延伸笔不改变
//   2. 无笔数上限（去掉 j < i+7）
//   3. 中枢不允许重叠（i = end_bi + 1）
//   4. 未完成笔（is_confirmed==false）不参与中枢计算
//   5. 中枢方向按第一笔方向确定
static void CheckZS(int min_zs_bi_count = 3) {
    g_Pivots.clear();
    
    // 排除未完成笔
    int n = (int)g_Strokes.size();
    if (n > 0 && !g_Strokes.back().is_confirmed) {
        n--;
    }
    if (n < min_zs_bi_count) return;
    
    int i = 0;
    while (i <= n - min_zs_bi_count) {
        float zg = 99999.0f;  // 中枢高点 = MIN(各笔高点)
        float zd = 0.0f;       // 中枢低点 = MAX(各笔低点)
        
        // 第一阶段：前三笔确定 ZG/ZD（不可变）
        for (int j = i; j < i + min_zs_bi_count && j < n; ++j) {
            float bi_high = std::max(g_Strokes[j].start_price, g_Strokes[j].end_price);
            float bi_low  = std::min(g_Strokes[j].start_price, g_Strokes[j].end_price);
            zg = std::min(zg, bi_high);
            zd = std::max(zd, bi_low);
        }
        
        if (zg <= zd) {
            ++i;
            continue;  // 前三笔无有效重叠
        }
        
        int zs_start = g_Strokes[i].start_idx;
        int zs_end = g_Strokes[i + min_zs_bi_count - 1].end_idx;
        int end_bi = i + min_zs_bi_count - 1;
        
        // 第二阶段：延伸 — 只检查是否与 [ZD, ZG] 有交集，不压缩区间
        for (int j = end_bi + 1; j < n; ++j) {
            float bi_high = std::max(g_Strokes[j].start_price, g_Strokes[j].end_price);
            float bi_low  = std::min(g_Strokes[j].start_price, g_Strokes[j].end_price);
            if (bi_high > zd && bi_low < zg) {
                end_bi = j;
                zs_end = g_Strokes[j].end_idx;
                // 注意：ZG 和 ZD 不变！
            } else {
                break;  // 脱离中枢区间
            }
        }
        
        // 构建 Pivot
        Pivot pivot;
        pivot.start_idx = zs_start;
        pivot.end_idx = zs_end;
        pivot.zg = zg;
        pivot.zd = zd;
        pivot.zz = (zg + zd) / 2.0f;
        pivot.direction = g_Strokes[i].direction;  // 中枢方向按第一笔方向
        
        g_Pivots.push_back(pivot);
        
        // 不允许中枢重叠：跳过整个中枢
        i = end_bi + 1;
    }
}

// ============================================================================
// 递归引用系统 - 构建 GG/DD/HH/LL 序列
// ============================================================================

// 获取指定K线位置的递归引用数据
static BiSequenceData GetBiSequence(int kline_idx) {
    BiSequenceData seq;
    memset(&seq, 0, sizeof(seq));
    
    // 初始化为无效值
    for (int i = 0; i < 6; ++i) {
        seq.GG[i] = 0;
        seq.DD[i] = 0;
        seq.HH[i] = 9999;
        seq.LL[i] = 9999;
    }
    
    if (g_Strokes.empty()) return seq;
    
    // 收集截至 kline_idx 的所有笔端点
    std::vector<std::pair<int, float>> tops;    // (索引, 价格) - 顶点
    std::vector<std::pair<int, float>> bottoms; // (索引, 价格) - 底点
    
    for (const Stroke& bi : g_Strokes) {
        if (!bi.is_confirmed) continue;  // 未完成笔不纳入递归引用
        if (bi.end_idx > kline_idx) break;
        
        if (bi.direction == 1) {
            // 向上笔：终点是顶
            tops.push_back({bi.end_idx, bi.end_price});
            // 起点是底（如果是第一笔）
            if (bottoms.empty() || bi.start_idx != bottoms.back().first) {
                bottoms.push_back({bi.start_idx, bi.start_price});
            }
        } else {
            // 向下笔：终点是底
            bottoms.push_back({bi.end_idx, bi.end_price});
            // 起点是顶
            if (tops.empty() || bi.start_idx != tops.back().first) {
                tops.push_back({bi.start_idx, bi.start_price});
            }
        }
    }
    
    // 填充 GG1-GG5（从最近到最远）
    int top_count = (int)tops.size();
    for (int i = 1; i <= 5 && i <= top_count; ++i) {
        int idx = top_count - i;
        seq.GG[i] = tops[idx].second;
        seq.HH[i] = kline_idx - tops[idx].first;
    }
    
    // 填充 DD1-DD5
    int bottom_count = (int)bottoms.size();
    for (int i = 1; i <= 5 && i <= bottom_count; ++i) {
        int idx = bottom_count - i;
        seq.DD[i] = bottoms[idx].second;
        seq.LL[i] = kline_idx - bottoms[idx].first;
    }
    
    // 判断方向：最后一笔已确认笔方向决定当前趋势后状态
    for (int si = (int)g_Strokes.size() - 1; si >= 0; --si) {
        const Stroke& last_bi = g_Strokes[si];
        if (!last_bi.is_confirmed) continue;  // 跳过未完成笔
        if (last_bi.end_idx <= kline_idx) {
            // 最后一笔向下 = 下跌后 = 方向1（适合找买点）
            // 最后一笔向上 = 上涨后 = 方向-1（适合找卖点）
            seq.direction = (last_bi.direction == -1) ? 1 : -1;
            break;
        }
    }
    
    return seq;
}

// ============================================================================
// 幅度计算函数
// ============================================================================

// 一买KJA条件（有缺口 + 幅度确认，成功率93%）
static bool CheckFirstBuyKJA(const BiSequenceData& seq) {
    if (seq.GG[3] == 0 || seq.DD[3] == 0) return false;
    
    float amp1 = seq.GG[3] - seq.DD[3];  // 第1段幅度
    float amp2 = seq.GG[2] - seq.DD[2];  // 第2段幅度
    float amp3 = seq.GG[1] - seq.DD[1];  // 第3段幅度
    
    return (amp3 < amp2) &&           // 第3段 < 第2段
           (amp3 > amp1) &&           // 第3段 > 第1段
           (amp1 < amp2) &&           // 第1段 < 第2段
           (amp2 > amp3 * 1.618f);    // 第2段 > 第3段 × φ
}

// 一买KJB条件（无缺口 + 幅度衰减，成功率85%）
static bool CheckFirstBuyKJB(const BiSequenceData& seq) {
    if (seq.GG[3] == 0 || seq.DD[3] == 0) return false;
    
    float amp1 = seq.GG[3] - seq.DD[3];
    float amp2 = seq.GG[2] - seq.DD[2];
    float amp3 = seq.GG[1] - seq.DD[1];
    
    return (amp1 > amp3) &&           // 第1段 > 第3段
           (amp1 > amp2) &&           // 第1段 > 第2段
           (amp2 < amp3);             // 第2段 < 第3段
}

// 一卖幅度条件（镜像）
static bool CheckFirstSellKJA(const BiSequenceData& seq) {
    if (seq.GG[3] == 0 || seq.DD[3] == 0) return false;
    
    float amp1 = seq.GG[3] - seq.DD[3];
    float amp2 = seq.GG[2] - seq.DD[2];
    float amp3 = seq.GG[1] - seq.DD[1];
    
    return (amp3 < amp2) && (amp3 > amp1) && (amp1 < amp2) && (amp2 > amp3 * 1.618f);
}

static bool CheckFirstSellKJB(const BiSequenceData& seq) {
    if (seq.GG[3] == 0 || seq.DD[3] == 0) return false;
    
    float amp1 = seq.GG[3] - seq.DD[3];
    float amp2 = seq.GG[2] - seq.DD[2];
    float amp3 = seq.GG[1] - seq.DD[1];
    
    return (amp1 > amp3) && (amp1 > amp2) && (amp2 < amp3);
}

// ============================================================================
// 买卖点判断函数（完全按照速查手册实现）
// ============================================================================

// 一买判断
// 返回值：1=一买A型, 2=一买B型, 0=不是一买
static int CheckFirstBuy(int kline_idx, float low, float ma13, const BiSequenceData& seq) {
    // 基础条件
    if (seq.direction != 1) return 0;           // 方向 = 1（下跌后）
    if (low >= ma13) return 0;                  // L < MA13
    if (seq.LL[1] > 5) return 0;                // LL1 ≤ 5
    
    // 形态条件（五段下跌）
    if (seq.DD[1] >= seq.GG[1]) return 0;       // DD1 < GG1
    if (seq.DD[1] >= seq.DD[2]) return 0;       // DD1 < DD2（创新低）
    if (seq.DD[1] >= seq.DD[3]) return 0;       // DD1 < DD3
    if (seq.GG[1] >= seq.GG[2]) return 0;       // GG1 < GG2
    if (seq.GG[1] >= seq.GG[3]) return 0;       // GG1 < GG3
    
    // 一买A型：存在缺口
    if (seq.GG[1] < seq.DD[3]) {
        return 1;  // 一买A
    }
    
    // 一买B型：无缺口但幅度衰减
    if (CheckFirstBuyKJA(seq) || CheckFirstBuyKJB(seq)) {
        return 2;  // 一买B
    }
    
    return 0;
}

// 二买判断
// 返回值：11=二买A型, 12=二买B1型, 13=二买B2型, 0=不是二买
static int CheckSecondBuy(int kline_idx, float low, float ma26, const BiSequenceData& seq) {
    // 基础条件
    if (seq.direction != 1) return 0;           // 方向 = 1
    if (low >= ma26) return 0;                  // L < MA26
    if (seq.LL[1] > 8) return 0;                // LL1 ≤ 8
    
    // 形态条件
    if (seq.DD[1] >= seq.GG[1]) return 0;       // DD1 < GG1（有反弹）
    if (seq.DD[1] <= seq.DD[2]) return 0;       // DD1 > DD2（底抬高！）
    
    // 二买A型：三浪下跌后
    if (seq.GG[3] > seq.GG[2] && seq.DD[3] > seq.DD[2]) {
        if (seq.GG[1] > seq.DD[3]) {            // 回到中枢
            return 11;  // 二买A
        }
    }
    
    // 二买B1/B2型：五浪下跌
    if (seq.GG[4] > 0 && seq.DD[4] > 0) {
        if (seq.GG[4] > seq.GG[3] && seq.GG[3] > seq.GG[2] &&
            seq.DD[2] < seq.DD[3] && seq.DD[3] < seq.DD[4]) {
            // 五浪下跌确认
            if (seq.GG[2] < seq.DD[4]) {
                // 存在缺口
                if (seq.GG[1] > seq.DD[3]) {
                    return 12;  // 二买B1
                }
            } else {
                // 无缺口
                return 13;  // 二买B2
            }
        }
    }
    
    return 0;
}

// 三买判断
// 返回值：21=三买, 0=不是三买
static int CheckThirdBuy(int kline_idx, float low, float ma13, const BiSequenceData& seq, const std::vector<Pivot>& pivots) {
    // 基础条件
    if (seq.direction != 1) return 0;           // 方向 = 1
    if (low >= ma13) return 0;                  // L < MA13
    if (seq.LL[1] > 5) return 0;                // LL1 ≤ 5
    
    // 形态条件
    if (seq.DD[1] >= seq.GG[1]) return 0;       // DD1 < GG1
    if (seq.DD[1] <= seq.DD[2]) return 0;       // DD1 > DD2（底抬高）
    
    // 中枢条件：DD1 > MIN(GG2, GG3) = 当前底高于前中枢上沿
    float zs_upper = std::min(seq.GG[2], seq.GG[3]);
    if (seq.DD[1] <= zs_upper) return 0;
    
    // GG3 > DD2 确保中枢存在
    if (seq.GG[3] <= seq.DD[2]) return 0;
    
    // DD4 < MAX(DD2, DD3) 前底低于中枢
    if (seq.DD[4] > 0) {
        float zs_max_low = std::max(seq.DD[2], seq.DD[3]);
        if (seq.DD[4] >= zs_max_low) return 0;
    }
    
    // DD1 > DD4 当前底高于突破前的底
    if (seq.DD[4] > 0 && seq.DD[1] <= seq.DD[4]) return 0;
    
    return 21;  // 三买
}

// 一卖判断（镜像对称）
// 返回值：-1=一卖A型, -2=一卖B型, -3=一卖C型, 0=不是一卖
static int CheckFirstSell(int kline_idx, float high, float ma13, const BiSequenceData& seq) {
    // 基础条件
    if (seq.direction != -1) return 0;          // 方向 = -1（上涨后）
    if (high <= ma13) return 0;                 // H > MA13
    if (seq.HH[1] > 5) return 0;                // HH1 ≤ 5
    
    // 形态条件（五段上涨）
    if (seq.GG[1] <= seq.DD[1]) return 0;       // GG1 > DD1
    if (seq.GG[1] <= seq.GG[2]) return 0;       // GG1 > GG2（创新高）
    if (seq.GG[1] <= seq.GG[3]) return 0;       // GG1 > GG3
    if (seq.DD[1] <= seq.DD[2]) return 0;       // DD1 > DD2
    if (seq.DD[1] <= seq.DD[3]) return 0;       // DD1 > DD3
    
    // 一卖A型：存在缺口
    if (seq.DD[1] > seq.GG[3]) {
        return -1;  // 一卖A
    }
    
    // 一卖B型：幅度衰减
    if (CheckFirstSellKJA(seq) || CheckFirstSellKJB(seq)) {
        return -2;  // 一卖B
    }
    
    // 一卖C型：连涨四段
    if (seq.GG[4] > 0 && seq.GG[5] > 0) {
        if (seq.GG[1] > seq.GG[2] && seq.GG[2] > seq.GG[3] && 
            seq.GG[3] > seq.GG[4] && seq.GG[4] > seq.GG[5]) {
            return -3;  // 一卖C
        }
    }
    
    return 0;
}

// 二卖判断
// 返回值：-11=二卖A型, -12=二卖B1型, -13=二卖B2型, -14=二卖C1型, 0=不是二卖
static int CheckSecondSell(int kline_idx, float high, float ma26, const BiSequenceData& seq) {
    // 基础条件
    if (seq.direction != -1) return 0;          // 方向 = -1
    if (high <= ma26) return 0;                 // H > MA26
    if (seq.HH[1] > 8) return 0;                // HH1 ≤ 8
    
    // 形态条件
    if (seq.GG[1] <= seq.DD[1]) return 0;       // GG1 > DD1
    if (seq.GG[1] >= seq.GG[2]) return 0;       // GG1 < GG2（顶降低！）
    
    // 二卖A型：三段上涨后
    if (seq.DD[3] < seq.DD[2] && seq.GG[3] < seq.GG[2]) {
        if (seq.DD[1] < seq.GG[3]) {            // 回到中枢
            return -11;  // 二卖A
        }
    }
    
    // 二卖B1/B2型：五段上涨
    if (seq.DD[4] > 0 && seq.GG[4] > 0) {
        if (seq.DD[4] < seq.DD[3] && seq.DD[3] < seq.DD[2] &&
            seq.GG[2] > seq.GG[3] && seq.GG[3] > seq.GG[4]) {
            if (seq.DD[2] > seq.GG[4]) {
                // 存在缺口
                if (seq.DD[1] < seq.GG[3]) {
                    return -12;  // 二卖B1
                }
            } else {
                return -13;  // 二卖B2
            }
        }
    }
    
    return 0;
}

// 三卖判断
// 返回值：-21=三卖, 0=不是三卖
static int CheckThirdSell(int kline_idx, float high, float ma13, const BiSequenceData& seq) {
    // 基础条件
    if (seq.direction != -1) return 0;          // 方向 = -1
    if (high <= ma13) return 0;                 // H > MA13
    if (seq.HH[1] > 5) return 0;                // HH1 ≤ 5
    
    // 形态条件
    if (seq.DD[1] >= seq.GG[1]) return 0;       // DD1 < GG1
    if (seq.GG[1] >= seq.GG[2]) return 0;       // GG1 < GG2（顶降低）
    
    // 中枢条件：GG1 < MAX(DD2, DD3) = 当前顶低于前中枢下沿
    float zs_lower = std::max(seq.DD[2], seq.DD[3]);
    if (seq.GG[1] >= zs_lower) return 0;
    
    // GG2 > DD3 确保中枢存在
    if (seq.GG[2] <= seq.DD[3]) return 0;
    
    // GG4 > MIN(GG2, GG3) 前顶高于中枢
    if (seq.GG[4] > 0) {
        float zs_min_high = std::min(seq.GG[2], seq.GG[3]);
        if (seq.GG[4] <= zs_min_high) return 0;
    }
    
    return -21;  // 三卖
}

// ============================================================================
// 准买卖点判断（条件放宽）
// ============================================================================

// 准一买：时间窗口放宽，形态不完整也可
static int CheckPreFirstBuy(int kline_idx, float low, float ma13, const BiSequenceData& seq) {
    if (seq.direction != 1) return 0;
    if (low >= ma13) return 0;
    if (seq.LL[1] > 8) return 0;  // 放宽至8
    
    // 部分底部降低即可
    if (seq.DD[1] < seq.DD[2] || seq.DD[1] < seq.DD[3]) {
        return 31;  // 准一买
    }
    return 0;
}

// 准二买
static int CheckPreSecondBuy(int kline_idx, float low, float ma26, const BiSequenceData& seq) {
    if (seq.direction != 1) return 0;
    if (low >= ma26) return 0;
    if (seq.LL[1] > 10) return 0;  // 放宽至10
    
    // 底抬高 + 中枢雏形
    if (seq.DD[1] > seq.DD[2]) {
        if (seq.GG[1] > seq.DD[2] || seq.GG[1] > seq.DD[3]) {
            return 32;  // 准二买
        }
    }
    return 0;
}

// 准三买
static int CheckPreThirdBuy(int kline_idx, float low, const BiSequenceData& seq) {
    if (seq.direction != 1) return 0;
    if (seq.DD[1] <= seq.DD[2]) return 0;  // 底抬高
    
    // 接近但未触及中枢上沿
    float zs_upper = std::min(seq.GG[2], seq.GG[3]);
    if (seq.DD[1] > zs_upper * 0.98f && seq.DD[1] <= zs_upper) {
        return 33;  // 准三买
    }
    return 0;
}

// 准卖点（镜像）
static int CheckPreFirstSell(int kline_idx, float high, float ma13, const BiSequenceData& seq) {
    if (seq.direction != -1) return 0;
    if (high <= ma13) return 0;
    if (seq.HH[1] > 8) return 0;
    
    if (seq.GG[1] > seq.GG[2] || seq.GG[1] > seq.GG[3]) {
        return -31;
    }
    return 0;
}

static int CheckPreSecondSell(int kline_idx, float high, float ma26, const BiSequenceData& seq) {
    if (seq.direction != -1) return 0;
    if (high <= ma26) return 0;
    if (seq.HH[1] > 10) return 0;
    
    if (seq.GG[1] < seq.GG[2]) {
        if (seq.DD[1] < seq.GG[2] || seq.DD[1] < seq.GG[3]) {
            return -32;
        }
    }
    return 0;
}

static int CheckPreThirdSell(int kline_idx, float high, const BiSequenceData& seq) {
    if (seq.direction != -1) return 0;
    if (seq.GG[1] >= seq.GG[2]) return 0;
    
    float zs_lower = std::max(seq.DD[2], seq.DD[3]);
    if (seq.GG[1] < zs_lower * 1.02f && seq.GG[1] >= zs_lower) {
        return -33;
    }
    return 0;
}

// ============================================================================
// 完整分析流程（带均线计算）
// ============================================================================

// [v7.5 TEMPORARY] 前向声明 — 回归基线导出（Phase 1 开始后删除）
static void DumpBaselineCSV(const float* highs, const float* lows, const float* closes, int count);

static void FullAnalyzeWithMA(const float* highs, const float* lows, const float* closes, int count) {
    // 检查是否需要重新计算
    if (count == g_LastCount && count > 0 &&
        highs[0] == g_LastHigh0 && lows[0] == g_LastLow0) {
        return;  // 数据未变，使用缓存
    }
    
    g_LastCount = count;
    g_LastHigh0 = (count > 0) ? highs[0] : 0;
    g_LastLow0 = (count > 0) ? lows[0] : 0;
    
    // 缓存收盘价
    g_Closes.resize(count);
    for (int i = 0; i < count; ++i) {
        g_Closes[i] = closes[i];
    }
    
    // 计算均线
    CalcMA(closes, count, 13, g_MA13);
    CalcMA(closes, count, 26, g_MA26);
    
    // 基础分析
    RemoveInclude(highs, lows, count);
    CheckFX();
    CheckBI(5);
    CheckZS(3);

    // [v7.5 TEMPORARY] 回归基线采集 — Phase 1 开始后删除此段
    DumpBaselineCSV(highs, lows, closes, count);
}

// ============================================================================
// [v7.5 TEMPORARY] 回归基线 CSV 导出 — Phase 1 开始后删除此函数
// ============================================================================
static bool g_BaselineDumped = false;

static void DumpBaselineCSV(const float* highs, const float* lows, const float* closes, int count) {
    if (g_BaselineDumped) return;
    g_BaselineDumped = true;

    char filename[MAX_PATH];
    sprintf(filename, "D:\\chan_baseline_%d.csv", count);

    FILE* fp = fopen(filename, "w");
    if (!fp) {
        WriteLog("[Baseline] ERROR: Failed to open output file");
        return;
    }

    // 分配临时数组
    std::vector<float> bi(count, 0.0f);
    std::vector<float> zszg(count, 0.0f);
    std::vector<float> zszd(count, 0.0f);
    std::vector<float> bsig(count, 0.0f);
    std::vector<float> ssig(count, 0.0f);
    std::vector<float> kxg(count, 0.0f);
    std::vector<float> kxd(count, 0.0f);

    // --- BI 笔端点 (同 BiDuanDian 逻辑) ---
    for (const Stroke& s : g_Strokes) {
        if (s.direction == 1) {
            if (s.start_idx >= 0 && s.start_idx < count) bi[s.start_idx] = -1.0f;
            if (s.end_idx >= 0 && s.end_idx < count) bi[s.end_idx] = 1.0f;
        } else if (s.direction == -1) {
            if (s.start_idx >= 0 && s.start_idx < count) bi[s.start_idx] = 1.0f;
            if (s.end_idx >= 0 && s.end_idx < count) bi[s.end_idx] = -1.0f;
        }
    }

    // --- ZSZG / ZSZD 中枢高低 (同 ZhongShuGao/ZhongShuDi 逻辑) ---
    {
        size_t pivotCount = g_Pivots.size();
        for (size_t p = 0; p < pivotCount; ++p) {
            const Pivot& zs = g_Pivots[p];
            int actual_end = zs.end_idx;
            if (p + 1 < pivotCount && zs.end_idx >= g_Pivots[p + 1].start_idx) {
                actual_end = g_Pivots[p + 1].start_idx - 1;
            }
            for (int i = zs.start_idx; i <= actual_end && i < count; ++i) {
                if (i >= 0) {
                    zszg[i] = zs.zg;
                    zszd[i] = zs.zd;
                }
            }
        }
    }

    // --- BSIG 买点信号 (同 BuySignal 逻辑) ---
    for (const Stroke& s : g_Strokes) {
        if (!s.is_confirmed) continue;
        if (s.direction != -1) continue;
        int idx = s.end_idx;
        if (idx < 0 || idx >= count) continue;
        float low = lows[idx];
        float ma13 = (idx < (int)g_MA13.size()) ? g_MA13[idx] : low;
        float ma26 = (idx < (int)g_MA26.size()) ? g_MA26[idx] : low;
        BiSequenceData seq = GetBiSequence(idx);
        int signal = 0;
        signal = CheckFirstBuy(idx, low, ma13, seq);
        if (signal > 0) { bsig[idx] = (float)signal; continue; }
        signal = CheckSecondBuy(idx, low, ma26, seq);
        if (signal > 0) { bsig[idx] = (float)signal; continue; }
        signal = CheckThirdBuy(idx, low, ma13, seq, g_Pivots);
        if (signal > 0) { bsig[idx] = (float)signal; continue; }
        signal = CheckPreFirstBuy(idx, low, ma13, seq);
        if (signal > 0) { bsig[idx] = (float)signal; continue; }
        signal = CheckPreSecondBuy(idx, low, ma26, seq);
        if (signal > 0) { bsig[idx] = (float)signal; continue; }
        signal = CheckPreThirdBuy(idx, low, seq);
        if (signal > 0) { bsig[idx] = (float)signal; continue; }
    }

    // --- SSIG 卖点信号 (同 SellSignal 逻辑) ---
    for (const Stroke& s : g_Strokes) {
        if (!s.is_confirmed) continue;
        if (s.direction != 1) continue;
        int idx = s.end_idx;
        if (idx < 0 || idx >= count) continue;
        float high = highs[idx];
        float ma13 = (idx < (int)g_MA13.size()) ? g_MA13[idx] : high;
        float ma26 = (idx < (int)g_MA26.size()) ? g_MA26[idx] : high;
        BiSequenceData seq = GetBiSequence(idx);
        int signal = 0;
        signal = CheckFirstSell(idx, high, ma13, seq);
        if (signal < 0) { ssig[idx] = (float)signal; continue; }
        signal = CheckSecondSell(idx, high, ma26, seq);
        if (signal < 0) { ssig[idx] = (float)signal; continue; }
        signal = CheckThirdSell(idx, high, ma13, seq);
        if (signal < 0) { ssig[idx] = (float)signal; continue; }
        signal = CheckPreFirstSell(idx, high, ma13, seq);
        if (signal < 0) { ssig[idx] = (float)signal; continue; }
        signal = CheckPreSecondSell(idx, high, ma26, seq);
        if (signal < 0) { ssig[idx] = (float)signal; continue; }
        signal = CheckPreThirdSell(idx, high, seq);
        if (signal < 0) { ssig[idx] = (float)signal; continue; }
    }

    // --- KXG 顶端点价格 (同 BiGaoDian 逻辑) ---
    for (const Stroke& s : g_Strokes) {
        if (s.direction == 1 && s.end_idx >= 0 && s.end_idx < count) {
            kxg[s.end_idx] = s.end_price;
        } else if (s.direction == -1 && s.start_idx >= 0 && s.start_idx < count) {
            kxg[s.start_idx] = s.start_price;
        }
    }

    // --- KXD 底端点价格 (同 BiDiDian 逻辑) ---
    for (const Stroke& s : g_Strokes) {
        if (s.direction == -1 && s.end_idx >= 0 && s.end_idx < count) {
            kxd[s.end_idx] = s.end_price;
        } else if (s.direction == 1 && s.start_idx >= 0 && s.start_idx < count) {
            kxd[s.start_idx] = s.start_price;
        }
    }

    // 写入 CSV
    fprintf(fp, "bar_index,BI,ZSZG,ZSZD,BSIG,SSIG,KXG,KXD\n");
    int nonZeroRows = 0;
    for (int i = 0; i < count; ++i) {
        if (bi[i] != 0 || zszg[i] != 0 || zszd[i] != 0 ||
            bsig[i] != 0 || ssig[i] != 0 || kxg[i] != 0 || kxd[i] != 0) {
            fprintf(fp, "%d,%.0f,%.4f,%.4f,%.0f,%.0f,%.4f,%.4f\n",
                    i, bi[i], zszg[i], zszd[i], bsig[i], ssig[i], kxg[i], kxd[i]);
            nonZeroRows++;
        }
    }
    fclose(fp);

    char logMsg[256];
    sprintf(logMsg, "[Baseline] Dumped %d non-zero rows (total %d bars) to %s",
            nonZeroRows, count, filename);
    WriteLog(logMsg);
}

// ============================================================================
// 通达信导出函数
// ============================================================================

// 函数1：分型识别
// 公式调用：FX:TDXDLL1(1, H, L, C);
// 返回值：1=顶分型, -1=底分型, 0=无
void FenXing(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;
    
    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);
    
    // 初始化输出
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    // 填充分型标记
    for (const Fractal& fx : g_Fractals) {
        if (fx.index >= 0 && fx.index < DataLen) {
            pfOUT[fx.index] = (float)fx.type;
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
    
    for (const Stroke& bi : g_Strokes) {
        // 向上笔：起点是底，终点是顶
        if (bi.direction == 1) {
            if (bi.start_idx >= 0 && bi.start_idx < DataLen) {
                pfOUT[bi.start_idx] = -1.0f;  // 底端点
            }
            if (bi.end_idx >= 0 && bi.end_idx < DataLen) {
                pfOUT[bi.end_idx] = 1.0f;     // 顶端点
            }
        }
        // 向下笔：起点是顶，终点是底
        else if (bi.direction == -1) {
            if (bi.start_idx >= 0 && bi.start_idx < DataLen) {
                pfOUT[bi.start_idx] = 1.0f;   // 顶端点
            }
            if (bi.end_idx >= 0 && bi.end_idx < DataLen) {
                pfOUT[bi.end_idx] = -1.0f;    // 底端点
            }
        }
    }
}

// 函数3：中枢高点
// 公式调用：ZS_H:TDXDLL1(3, H, L, C);
void ZhongShuGao(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;
    
    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);
    
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    size_t pivotCount = g_Pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const Pivot& zs = g_Pivots[p];
        int actual_end = zs.end_idx;
        // 边界收缩：如果当前中枢end与下一个中枢start重叠，收缩1格避免覆盖
        if (p + 1 < pivotCount && zs.end_idx >= g_Pivots[p + 1].start_idx) {
            actual_end = g_Pivots[p + 1].start_idx - 1;
        }
        for (int i = zs.start_idx; i <= actual_end && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = zs.zg;
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
    
    size_t pivotCount = g_Pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const Pivot& zs = g_Pivots[p];
        int actual_end = zs.end_idx;
        // 边界收缩：如果当前中枢end与下一个中枢start重叠，收缩1格避免覆盖
        if (p + 1 < pivotCount && zs.end_idx >= g_Pivots[p + 1].start_idx) {
            actual_end = g_Pivots[p + 1].start_idx - 1;
        }
        for (int i = zs.start_idx; i <= actual_end && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = zs.zd;
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
    
    size_t pivotCount = g_Pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const Pivot& zs = g_Pivots[p];
        int actual_end = zs.end_idx;
        // 边界收缩：如果当前中枢end与下一个中枢start重叠，收缩1格避免覆盖
        if (p + 1 < pivotCount && zs.end_idx >= g_Pivots[p + 1].start_idx) {
            actual_end = g_Pivots[p + 1].start_idx - 1;
        }
        for (int i = zs.start_idx; i <= actual_end && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = zs.zz;
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
    
    for (const Stroke& bi : g_Strokes) {
        for (int i = bi.start_idx; i <= bi.end_idx && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = (float)bi.direction;
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
    for (const Stroke& bi : g_Strokes) {
        if (!bi.is_confirmed) continue;    // 跳过未完成笔
        if (bi.direction != -1) continue;  // 只检查向下笔终点
        
        int idx = bi.end_idx;
        if (idx < 0 || idx >= DataLen) continue;
        
        float low = pfINb[idx];
        float ma13 = (idx < (int)g_MA13.size()) ? g_MA13[idx] : low;
        float ma26 = (idx < (int)g_MA26.size()) ? g_MA26[idx] : low;
        
        // 获取递归引用数据
        BiSequenceData seq = GetBiSequence(idx);
        
        // 按优先级检查：一买 > 二买 > 三买 > 准买点
        int signal = 0;
        
        // 一买检查
        signal = CheckFirstBuy(idx, low, ma13, seq);
        if (signal > 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        // 二买检查
        signal = CheckSecondBuy(idx, low, ma26, seq);
        if (signal > 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        // 三买检查
        signal = CheckThirdBuy(idx, low, ma13, seq, g_Pivots);
        if (signal > 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        // 准买点检查（优先级最低）
        signal = CheckPreFirstBuy(idx, low, ma13, seq);
        if (signal > 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        signal = CheckPreSecondBuy(idx, low, ma26, seq);
        if (signal > 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        signal = CheckPreThirdBuy(idx, low, seq);
        if (signal > 0) {
            pfOUT[idx] = (float)signal;
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
    for (const Stroke& bi : g_Strokes) {
        if (!bi.is_confirmed) continue;   // 跳过未完成笔
        if (bi.direction != 1) continue;  // 只检查向上笔终点
        
        int idx = bi.end_idx;
        if (idx < 0 || idx >= DataLen) continue;
        
        float high = pfINa[idx];
        float ma13 = (idx < (int)g_MA13.size()) ? g_MA13[idx] : high;
        float ma26 = (idx < (int)g_MA26.size()) ? g_MA26[idx] : high;
        
        // 获取递归引用数据
        BiSequenceData seq = GetBiSequence(idx);
        
        // 按优先级检查：一卖 > 二卖 > 三卖 > 准卖点
        int signal = 0;
        
        // 一卖检查
        signal = CheckFirstSell(idx, high, ma13, seq);
        if (signal < 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        // 二卖检查
        signal = CheckSecondSell(idx, high, ma26, seq);
        if (signal < 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        // 三卖检查
        signal = CheckThirdSell(idx, high, ma13, seq);
        if (signal < 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        // 准卖点检查
        signal = CheckPreFirstSell(idx, high, ma13, seq);
        if (signal < 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        signal = CheckPreSecondSell(idx, high, ma26, seq);
        if (signal < 0) {
            pfOUT[idx] = (float)signal;
            continue;
        }
        
        signal = CheckPreThirdSell(idx, high, seq);
        if (signal < 0) {
            pfOUT[idx] = (float)signal;
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
    
    // 默认全部为0（被合并）
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    // 标记保留的K线
    for (const MergedKLine& mk : g_MergedKLines) {
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
        BiSequenceData seq = GetBiSequence(i);
        pfOUT[i] = (float)seq.direction;
    }
}

// 函数12：GG1 - 最近顶点价格
// 公式调用：GG1:TDXDLL1(12, H, L, C);
void OutputGG1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;
    
    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);
    
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    for (int i = 0; i < DataLen; ++i) {
        BiSequenceData seq = GetBiSequence(i);
        pfOUT[i] = seq.GG[1];
    }
}

// 函数13：DD1 - 最近底点价格
// 公式调用：DD1:TDXDLL1(13, H, L, C);
void OutputDD1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;
    
    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);
    
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    for (int i = 0; i < DataLen; ++i) {
        BiSequenceData seq = GetBiSequence(i);
        pfOUT[i] = seq.DD[1];
    }
}

// 函数14：LL1 - 最近底点距当前K线数
// 公式调用：LL1:TDXDLL1(14, H, L, C);
void OutputLL1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;
    
    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);
    
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    for (int i = 0; i < DataLen; ++i) {
        BiSequenceData seq = GetBiSequence(i);
        pfOUT[i] = (float)seq.LL[1];
    }
}

// 函数15：HH1 - 最近顶点距当前K线数
// 公式调用：HH1:TDXDLL1(15, H, L, C);
void OutputHH1(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    if (!pfOUT || DataLen <= 0) return;
    
    FullAnalyzeWithMA(pfINa, pfINb, pfINc, DataLen);
    
    memset(pfOUT, 0, DataLen * sizeof(float));
    
    for (int i = 0; i < DataLen; ++i) {
        BiSequenceData seq = GetBiSequence(i);
        pfOUT[i] = (float)seq.HH[1];
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
    
    for (const Pivot& zs : g_Pivots) {
        if (zs.start_idx >= 0 && zs.start_idx < DataLen) {
            // 1=下跌中枢(第一笔向下), 2=上涨中枢(第一笔向上)
            pfOUT[zs.start_idx] = (zs.direction == -1) ? 1.0f : 2.0f;
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
    
    size_t pivotCount = g_Pivots.size();
    for (size_t p = 0; p < pivotCount; ++p) {
        const Pivot& zs = g_Pivots[p];
        int actual_end = zs.end_idx;
        // 边界收缩：如果当前中枢end与下一个中枢start重叠，收缩1格避免覆盖
        if (p + 1 < pivotCount && zs.end_idx >= g_Pivots[p + 1].start_idx) {
            actual_end = g_Pivots[p + 1].start_idx - 1;
        }
        if (actual_end >= 0 && actual_end < DataLen) {
            pfOUT[actual_end] = (zs.direction == -1) ? 1.0f : 2.0f;
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
    for (const Stroke& bi : g_Strokes) {
        if (bi.direction == 1) {
            // 向上笔的终点是顶点
            top_indices.insert(bi.end_idx);
        } else if (bi.direction == -1) {
            // 向下笔的起点是顶点
            top_indices.insert(bi.start_idx);
        }
    }
    
    // 在顶点位置输出高点价格
    for (int idx : top_indices) {
        if (idx >= 0 && idx < DataLen) {
            // 找到对应笔的价格
            for (const Stroke& bi : g_Strokes) {
                if (bi.direction == 1 && bi.end_idx == idx) {
                    pfOUT[idx] = bi.end_price;
                    break;
                } else if (bi.direction == -1 && bi.start_idx == idx) {
                    pfOUT[idx] = bi.start_price;
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
    for (const Stroke& bi : g_Strokes) {
        if (bi.direction == -1) {
            // 向下笔的终点是底点
            bottom_indices.insert(bi.end_idx);
        } else if (bi.direction == 1) {
            // 向上笔的起点是底点
            bottom_indices.insert(bi.start_idx);
        }
    }
    
    // 在底点位置输出低点价格
    for (int idx : bottom_indices) {
        if (idx >= 0 && idx < DataLen) {
            for (const Stroke& bi : g_Strokes) {
                if (bi.direction == -1 && bi.end_idx == idx) {
                    pfOUT[idx] = bi.end_price;
                    break;
                } else if (bi.direction == 1 && bi.start_idx == idx) {
                    pfOUT[idx] = bi.start_price;
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
    
    for (const Pivot& zs : g_Pivots) {
        for (int i = zs.start_idx; i <= zs.end_idx && i < DataLen; ++i) {
            if (i >= 0) {
                pfOUT[i] = (float)zs.direction;
            }
        }
    }
}

// ============================================================================
// 函数注册数组
// ============================================================================

PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1,  (pPluginFUNC)&FenXing},        // 分型
    {2,  (pPluginFUNC)&BiDuanDian},     // 笔端点
    {3,  (pPluginFUNC)&ZhongShuGao},    // 中枢高
    {4,  (pPluginFUNC)&ZhongShuDi},     // 中枢低
    {5,  (pPluginFUNC)&ZhongShuZhong},  // 中枢中轴
    {6,  (pPluginFUNC)&BiDirection},    // 笔方向
    {7,  (pPluginFUNC)&BuySignal},      // 买点（完整版）
    {8,  (pPluginFUNC)&SellSignal},     // 卖点（完整版）
    {9,  (pPluginFUNC)&NewBar},         // 新K线
    {10, (pPluginFUNC)&TestFunc},       // 测试
    {11, (pPluginFUNC)&Direction},      // 方向
    {12, (pPluginFUNC)&OutputGG1},      // GG1
    {13, (pPluginFUNC)&OutputDD1},      // DD1
    {14, (pPluginFUNC)&OutputLL1},      // LL1
    {15, (pPluginFUNC)&OutputHH1},      // HH1
    {16, (pPluginFUNC)&OutputMA13},     // MA13
    {17, (pPluginFUNC)&OutputMA26},     // MA26
    {18, (pPluginFUNC)&ZhongShuKaiShi}, // 中枢开始
    {19, (pPluginFUNC)&ZhongShuJieShu}, // 中枢结束
    {20, (pPluginFUNC)&BiGaoDian},      // 笔高点
    {21, (pPluginFUNC)&BiDiDian},       // 笔低点
    {22, (pPluginFUNC)&ZhongShuFangXiang}, // 中枢方向
    {0,  NULL}  // 结束标记
};

// ============================================================================
// 导出函数
// ============================================================================

extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    WriteLog("RegisterTdxFunc v7.4 - 新增中枢方向输出");
    
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
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        WriteLog("=== chan.dll v6.0 完整版加载 ===");
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
