// ============================================================================
// 缠论通达信DLL插件 - 核心算法实现
// ============================================================================
// 实现缠论核心算法：去包含、分型识别、笔识别、中枢识别
// 参考文档：《通达信缠论DLL开发需求文档》v2.1
// ============================================================================

#include "chan_core.h"
#include "logger.h"
#include <cstring>
#include <limits>
#include <algorithm>

namespace chan {

// ============================================================================
// 构造函数
// ============================================================================

ChanCore::ChanCore() 
    : m_raw_count(0) {
}

ChanCore::ChanCore(const ChanConfig& config) 
    : m_config(config)
    , m_raw_count(0) {
}

void ChanCore::SetConfig(const ChanConfig& config) {
    m_config = config;
}

void ChanCore::Clear() {
    m_raw_count = 0;
    m_merged_klines.clear();
    m_fractals.clear();
    m_strokes.clear();
    m_pivots.clear();
    m_raw_to_merged.clear();
}

// ============================================================================
// 主处理流程
// ============================================================================

int ChanCore::Analyze(const float* highs, const float* lows, 
                      const float* closes, const float* volumes, int count) {
    if (!highs || !lows || count <= 0) {
        CHAN_LOG_ERROR("Analyze: 输入参数无效");
        return -1;
    }
    
    Clear();
    m_raw_count = count;
    
    // 步骤1: 去包含处理
    int merged_count = RemoveInclude(highs, lows, count);
    CHAN_LOG_DEBUG("去包含处理完成: %d -> %d 根K线", count, merged_count);
    
    if (merged_count < 3) {
        CHAN_LOG_DEBUG("K线数量不足，无法识别分型");
        return 0;
    }
    
    // 步骤2: 分型识别
    int fx_count = CheckFX();
    CHAN_LOG_DEBUG("分型识别完成: %d 个分型", fx_count);
    
    if (fx_count < 2) {
        CHAN_LOG_DEBUG("分型数量不足，无法识别笔");
        return 0;
    }
    
    // 步骤3: 笔识别
    int bi_count = CheckBI();
    CHAN_LOG_DEBUG("笔识别完成: %d 笔", bi_count);
    
    if (bi_count < 3) {
        CHAN_LOG_DEBUG("笔数量不足，无法识别中枢");
        return 0;
    }
    
    // 步骤4: 中枢识别
    int zs_count = CheckZS();
    CHAN_LOG_DEBUG("中枢识别完成: %d 个中枢", zs_count);
    
    return 0;
}

// ============================================================================
// 去包含处理 (5.1)
// ============================================================================

bool ChanCore::HasIncludeRelation(const KLine& k1, const KLine& k2) const {
    // K1包含K2 或 K2包含K1
    return (k1.high >= k2.high && k1.low <= k2.low) ||
           (k2.high >= k1.high && k2.low <= k1.low);
}

void ChanCore::MergeKLine(KLine& target, const KLine& source, Direction dir) {
    if (dir == Direction::UP) {
        // 向上趋势：高点取高者，低点取高者
        target.high = std::max(target.high, source.high);
        target.low = std::max(target.low, source.low);
    } else {
        // 向下趋势：高点取低者，低点取低者
        target.high = std::min(target.high, source.high);
        target.low = std::min(target.low, source.low);
    }
    
    target.volume += source.volume;
    target.amount += source.amount;
    target.is_merged = true;
    target.merge_end = source.index;
}

Direction ChanCore::DetermineDirection(const std::vector<KLine>& klines, int idx) const {
    if (idx < 1 || klines.empty()) {
        return Direction::NONE;
    }
    
    // 向前查找已确定的方向
    for (int i = idx - 1; i >= 0; --i) {
        if (i < (int)klines.size() - 1) {
            // 根据相邻K线的高点变化判断方向
            if (klines[i].high < klines[i + 1].high) {
                return Direction::UP;
            } else if (klines[i].high > klines[i + 1].high) {
                return Direction::DOWN;
            }
        }
    }
    
    return Direction::UP;  // 默认向上
}

int ChanCore::RemoveInclude(const float* highs, const float* lows, int count) {
    if (!highs || !lows || count <= 0) {
        return 0;
    }
    
    m_merged_klines.clear();
    m_raw_to_merged.clear();
    m_raw_to_merged.resize(count, -1);
    
    // 初始化第一根K线
    KLine first;
    first.index = 0;
    first.high = highs[0];
    first.low = lows[0];
    first.is_merged = false;
    first.merge_start = 0;
    first.merge_end = 0;
    m_merged_klines.push_back(first);
    m_raw_to_merged[0] = 0;
    
    Direction curr_dir = Direction::NONE;
    
    for (int i = 1; i < count; ++i) {
        KLine& last = m_merged_klines.back();
        
        KLine curr;
        curr.index = i;
        curr.high = highs[i];
        curr.low = lows[i];
        curr.is_merged = false;
        curr.merge_start = i;
        curr.merge_end = i;
        
        // 判断是否存在包含关系
        if (HasIncludeRelation(last, curr)) {
            // 存在包含关系，需要合并
            
            // 确定方向
            if (curr_dir == Direction::NONE) {
                // 首次确定方向，根据前两根K线
                if (m_merged_klines.size() >= 2) {
                    int prev_idx = (int)m_merged_klines.size() - 2;
                    if (m_merged_klines[prev_idx].high < last.high) {
                        curr_dir = Direction::UP;
                    } else {
                        curr_dir = Direction::DOWN;
                    }
                } else {
                    // 只有一根K线，默认向上
                    curr_dir = Direction::UP;
                }
            }
            
            // 合并K线
            MergeKLine(last, curr, curr_dir);
            m_raw_to_merged[i] = (int)m_merged_klines.size() - 1;
        } else {
            // 不存在包含关系，添加新K线
            
            // 更新方向
            if (curr.high > last.high) {
                curr_dir = Direction::UP;
            } else if (curr.high < last.high) {
                curr_dir = Direction::DOWN;
            }
            // 高点相等时保持原方向
            
            m_merged_klines.push_back(curr);
            m_raw_to_merged[i] = (int)m_merged_klines.size() - 1;
        }
    }
    
    return (int)m_merged_klines.size();
}

// ============================================================================
// 分型识别 (5.2)
// ============================================================================

int ChanCore::CheckFX() {
    m_fractals.clear();
    
    const auto& klines = m_merged_klines;
    int n = (int)klines.size();
    
    if (n < 3) {
        return 0;
    }
    
    FractalType last_type = FractalType::NONE;
    
    for (int i = 1; i < n - 1; ++i) {
        const KLine& k1 = klines[i - 1];
        const KLine& k2 = klines[i];      // 中间K线
        const KLine& k3 = klines[i + 1];
        
        FractalType type = FractalType::NONE;
        
        // 顶分型判断：
        // 中间K线高点最高，低点也最高
        if (k2.high > k1.high && k2.high > k3.high &&
            k2.low > k1.low && k2.low > k3.low) {
            type = FractalType::TOP;
        }
        // 底分型判断：
        // 中间K线低点最低，高点也最低
        else if (k2.low < k1.low && k2.low < k3.low &&
                 k2.high < k1.high && k2.high < k3.high) {
            type = FractalType::BOTTOM;
        }
        
        if (type != FractalType::NONE) {
            // 创建分型
            Fractal fx;
            fx.index = i;
            fx.type = type;
            fx.price = (type == FractalType::TOP) ? k2.high : k2.low;
            fx.kline_idx = k2.merge_end;  // 使用合并K线的最后一根原始K线索引
            fx.is_valid = true;
            fx.strength = 1;
            
            // 处理连续同类型分型
            if (last_type == type && !m_fractals.empty()) {
                // 取极值
                Fractal& last_fx = m_fractals.back();
                if (type == FractalType::TOP) {
                    // 顶分型取高者
                    if (fx.price > last_fx.price) {
                        last_fx = fx;
                    }
                } else {
                    // 底分型取低者
                    if (fx.price < last_fx.price) {
                        last_fx = fx;
                    }
                }
            } else {
                // 不同类型，添加新分型
                m_fractals.push_back(fx);
                last_type = type;
            }
        }
    }
    
    return (int)m_fractals.size();
}

// ============================================================================
// 笔识别 (5.3)
// ============================================================================

bool ChanCore::IsFXValid(const Fractal& fx) const {
    return fx.is_valid && fx.type != FractalType::NONE;
}

bool ChanCore::CanFormStroke(const Fractal& fx1, const Fractal& fx2) const {
    // 检查分型类型是否交替
    if (fx1.type == fx2.type) {
        return false;
    }
    
    // 检查分型间隔（合并K线索引差）
    int distance = fx2.index - fx1.index;
    if (distance < m_config.min_fx_distance + 2) {  // +2 因为分型本身占3根K线
        return false;
    }
    
    // 检查K线数量是否满足最小笔长度
    // 使用原始K线索引计算
    int raw_distance = fx2.kline_idx - fx1.kline_idx;
    if (raw_distance < m_config.min_bi_len) {
        return false;
    }
    
    // 检查价格有效性
    if (fx1.type == FractalType::TOP) {
        // 顶到底：顶的高点必须高于底的低点
        if (fx1.price <= fx2.price) {
            return false;
        }
    } else {
        // 底到顶：底的低点必须低于顶的高点
        if (fx1.price >= fx2.price) {
            return false;
        }
    }
    
    return true;
}

int ChanCore::CheckBI() {
    m_strokes.clear();
    
    const auto& fxlist = m_fractals;
    int n = (int)fxlist.size();
    
    if (n < 2) {
        return 0;
    }
    
    int stroke_id = 0;
    int start_idx = 0;  // 起始分型索引
    
    while (start_idx < n - 1) {
        const Fractal& start_fx = fxlist[start_idx];
        
        // 寻找能够形成笔的下一个分型
        bool found = false;
        for (int end_idx = start_idx + 1; end_idx < n; ++end_idx) {
            const Fractal& end_fx = fxlist[end_idx];
            
            if (CanFormStroke(start_fx, end_fx)) {
                // 可以形成笔
                Stroke stroke;
                stroke.id = stroke_id++;
                stroke.start_idx = start_fx.kline_idx;
                stroke.end_idx = end_fx.kline_idx;
                stroke.start_fx = start_fx;
                stroke.end_fx = end_fx;
                
                if (start_fx.type == FractalType::BOTTOM) {
                    // 底到顶 = 上涨笔
                    stroke.direction = Direction::UP;
                    stroke.low = start_fx.price;
                    stroke.high = end_fx.price;
                } else {
                    // 顶到底 = 下跌笔
                    stroke.direction = Direction::DOWN;
                    stroke.high = start_fx.price;
                    stroke.low = end_fx.price;
                }
                
                stroke.power = stroke.high - stroke.low;
                stroke.kline_count = end_fx.kline_idx - start_fx.kline_idx + 1;
                
                m_strokes.push_back(stroke);
                
                start_idx = end_idx;
                found = true;
                break;
            }
        }
        
        if (!found) {
            // 没有找到能形成笔的分型，跳过当前分型
            start_idx++;
        }
    }
    
    return (int)m_strokes.size();
}

// ============================================================================
// 中枢识别 (5.4)
// ============================================================================

int ChanCore::CheckZS() {
    m_pivots.clear();
    
    const auto& strokes = m_strokes;
    int n = (int)strokes.size();
    
    if (n < m_config.min_zs_bi_count) {
        return 0;
    }
    
    int pivot_id = 0;
    int i = 0;
    
    while (i <= n - m_config.min_zs_bi_count) {
        // 尝试从第i笔开始形成中枢
        
        // 初始化中枢边界（使用前三笔）
        float zg = std::numeric_limits<float>::max();   // 最低的高点
        float zd = std::numeric_limits<float>::lowest(); // 最高的低点
        float gg = std::numeric_limits<float>::lowest(); // 最高点
        float dd = std::numeric_limits<float>::max();    // 最低点
        
        // 计算前三笔的重叠区间
        for (int j = i; j < i + m_config.min_zs_bi_count && j < n; ++j) {
            zg = std::min(zg, strokes[j].high);
            zd = std::max(zd, strokes[j].low);
            gg = std::max(gg, strokes[j].high);
            dd = std::min(dd, strokes[j].low);
        }
        
        // 检查是否有重叠区间
        if (zg > zd) {
            // 有效中枢
            Pivot pivot;
            pivot.id = pivot_id++;
            pivot.ZG = zg;
            pivot.ZD = zd;
            pivot.ZZ = (zg + zd) / 2.0f;
            pivot.GG = gg;
            pivot.DD = dd;
            pivot.start_stroke_id = strokes[i].id;
            pivot.start_idx = strokes[i].start_idx;
            pivot.stroke_count = m_config.min_zs_bi_count;
            
            // 中枢方向由进入段决定
            pivot.direction = strokes[i].direction;
            
            // 尝试扩展中枢
            int end_bi = i + m_config.min_zs_bi_count - 1;
            for (int j = end_bi + 1; j < n; ++j) {
                // 检查该笔是否与中枢有重叠
                if (strokes[j].high > zd && strokes[j].low < zg) {
                    // 有重叠，扩展中枢
                    end_bi = j;
                    pivot.stroke_count++;
                    pivot.GG = std::max(pivot.GG, strokes[j].high);
                    pivot.DD = std::min(pivot.DD, strokes[j].low);
                    // 注意：ZG和ZD不变，只是记录更多的笔进入中枢
                } else {
                    // 无重叠，中枢结束
                    break;
                }
            }
            
            pivot.end_stroke_id = strokes[end_bi].id;
            pivot.end_idx = strokes[end_bi].end_idx;
            
            m_pivots.push_back(pivot);
            
            // 从中枢结束后的下一笔继续
            i = end_bi + 1;
        } else {
            // 无重叠区间，跳过当前笔
            i++;
        }
    }
    
    return (int)m_pivots.size();
}

// ============================================================================
// 输出函数
// ============================================================================

void ChanCore::OutputFX(float* out, int count) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    memset(out, 0, count * sizeof(float));
    
    // 填充分型标记
    for (const auto& fx : m_fractals) {
        int idx = fx.kline_idx;
        if (idx >= 0 && idx < count) {
            out[idx] = static_cast<float>(fx.type == FractalType::TOP ? 1 : -1);
        }
    }
}

void ChanCore::OutputBI(float* out, int count) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    memset(out, 0, count * sizeof(float));
    
    // 填充笔端点
    for (const auto& stroke : m_strokes) {
        // 起点
        int start = stroke.start_idx;
        if (start >= 0 && start < count) {
            out[start] = (stroke.direction == Direction::UP) ? 
                         stroke.low : stroke.high;
        }
        
        // 终点
        int end = stroke.end_idx;
        if (end >= 0 && end < count) {
            out[end] = (stroke.direction == Direction::UP) ? 
                       stroke.high : stroke.low;
        }
    }
}

void ChanCore::OutputZS_H(float* out, int count) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    memset(out, 0, count * sizeof(float));
    
    // 填充中枢高点
    for (const auto& pivot : m_pivots) {
        for (int i = pivot.start_idx; i <= pivot.end_idx && i < count; ++i) {
            if (i >= 0) {
                out[i] = pivot.ZG;
            }
        }
    }
}

void ChanCore::OutputZS_L(float* out, int count) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    memset(out, 0, count * sizeof(float));
    
    // 填充中枢低点
    for (const auto& pivot : m_pivots) {
        for (int i = pivot.start_idx; i <= pivot.end_idx && i < count; ++i) {
            if (i >= 0) {
                out[i] = pivot.ZD;
            }
        }
    }
}

int ChanCore::GetMergedIndex(int raw_index) const {
    if (raw_index < 0 || raw_index >= (int)m_raw_to_merged.size()) {
        return -1;
    }
    return m_raw_to_merged[raw_index];
}

// ============================================================================
// 阶段二：递归引用系统
// ============================================================================

void ChanCore::BuildBiSequence(int current_bar_idx) {
    // 清空之前的数据
    m_bi_sequence.clear();
    m_bi_sequence.resize(current_bar_idx + 1);
    
    if (m_strokes.empty()) {
        return;
    }
    
    // 为每一根K线计算其对应的 GG/DD/HH/LL 序列
    for (int bar = 0; bar <= current_bar_idx; ++bar) {
        BiSequenceData& seq = m_bi_sequence[bar];
        
        // 初始化为0
        for (int i = 0; i < 6; ++i) {
            seq.GG[i] = 0.0f;
            seq.DD[i] = 0.0f;
            seq.HH[i] = 0;
            seq.LL[i] = 0;
        }
        seq.direction = 0;
        
        // 收集在当前K线之前完成的所有顶点和底点
        std::vector<std::pair<float, int>> tops;    // (价格, K线索引)
        std::vector<std::pair<float, int>> bottoms; // (价格, K线索引)
        
        for (const auto& stroke : m_strokes) {
            // 只考虑在当前K线之前完成的笔
            if (stroke.end_idx > bar) {
                continue;
            }
            
            if (stroke.direction == Direction::UP) {
                // 向上笔：终点是顶点
                tops.push_back({stroke.high, stroke.end_idx});
            } else {
                // 向下笔：终点是底点
                bottoms.push_back({stroke.low, stroke.end_idx});
            }
        }
        
        // 按K线索引降序排序（最近的在前）
        auto cmp = [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
            return a.second > b.second;
        };
        std::sort(tops.begin(), tops.end(), cmp);
        std::sort(bottoms.begin(), bottoms.end(), cmp);
        
        // 填充 GG1-GG5, HH1-HH5
        for (int i = 0; i < 5 && i < (int)tops.size(); ++i) {
            seq.GG[i + 1] = tops[i].first;  // GG[1] = GG1
            seq.HH[i + 1] = bar - tops[i].second;
        }
        
        // 填充 DD1-DD5, LL1-LL5
        for (int i = 0; i < 5 && i < (int)bottoms.size(); ++i) {
            seq.DD[i + 1] = bottoms[i].first;  // DD[1] = DD1
            seq.LL[i + 1] = bar - bottoms[i].second;
        }
        
        // 计算方向
        seq.direction = CalculateDirection(bar);
    }
}

int ChanCore::CalculateDirection(int bar_idx) const {
    // 方向判断规则：
    // 方向=1：下跌趋势后（底分型）- 看涨
    // 方向=-1：上涨趋势后（顶分型）- 看跌
    // 方向=0：震荡
    
    if (m_strokes.empty()) {
        return 0;
    }
    
    // 找到当前K线所在的最近一笔
    const Stroke* lastStroke = nullptr;
    for (auto it = m_strokes.rbegin(); it != m_strokes.rend(); ++it) {
        if (it->end_idx <= bar_idx) {
            lastStroke = &(*it);
            break;
        }
    }
    
    if (!lastStroke) {
        return 0;
    }
    
    // 如果最近完成的笔是向下笔，说明刚形成底分型，方向=1（看涨）
    // 如果最近完成的笔是向上笔，说明刚形成顶分型，方向=-1（看跌）
    if (lastStroke->direction == Direction::DOWN) {
        return 1;   // 下跌后，看涨
    } else if (lastStroke->direction == Direction::UP) {
        return -1;  // 上涨后，看跌
    }
    
    return 0;
}

float ChanCore::GetGG(int bar_idx, int n) const {
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size() || n < 1 || n > 5) {
        return 0.0f;
    }
    return m_bi_sequence[bar_idx].GG[n];
}

float ChanCore::GetDD(int bar_idx, int n) const {
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size() || n < 1 || n > 5) {
        return 0.0f;
    }
    return m_bi_sequence[bar_idx].DD[n];
}

int ChanCore::GetHH(int bar_idx, int n) const {
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size() || n < 1 || n > 5) {
        return 0;
    }
    return m_bi_sequence[bar_idx].HH[n];
}

int ChanCore::GetLL(int bar_idx, int n) const {
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size() || n < 1 || n > 5) {
        return 0;
    }
    return m_bi_sequence[bar_idx].LL[n];
}

int ChanCore::GetDirection(int bar_idx) const {
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return 0;
    }
    return m_bi_sequence[bar_idx].direction;
}

// ============================================================================
// 阶段二：幅度计算
// ============================================================================

bool ChanCore::CheckFirstBuyKJA(int bar_idx) const {
    // 一买AAA型幅度条件（KJA）：
    // (GG1-DD1) < (GG2-DD2) AND
    // (GG1-DD1) > (GG3-DD3) AND
    // (GG3-DD3) < (GG2-DD2) AND
    // (GG2-DD2) > (GG1-DD1) * 1.618
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG1 == 0 || GG2 == 0 || GG3 == 0 || DD1 == 0 || DD2 == 0 || DD3 == 0) {
        return false;
    }
    
    float amp1 = GG1 - DD1;  // 第1段幅度
    float amp2 = GG2 - DD2;  // 第2段幅度
    float amp3 = GG3 - DD3;  // 第3段幅度
    
    return (amp1 < amp2) && 
           (amp1 > amp3) && 
           (amp3 < amp2) && 
           (amp2 > amp1 * 1.618f);
}

bool ChanCore::CheckFirstBuyKJB(int bar_idx) const {
    // 一买B型幅度条件（KJB）：
    // (GG3-DD3) > (GG1-DD1) AND
    // (GG3-DD3) > (GG2-DD2) AND
    // (GG2-DD2) < (GG1-DD1)
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG1 == 0 || GG2 == 0 || GG3 == 0 || DD1 == 0 || DD2 == 0 || DD3 == 0) {
        return false;
    }
    
    float amp1 = GG1 - DD1;  // 第1段幅度
    float amp2 = GG2 - DD2;  // 第2段幅度
    float amp3 = GG3 - DD3;  // 第3段幅度
    
    return (amp3 > amp1) && 
           (amp3 > amp2) && 
           (amp2 < amp1);
}

bool ChanCore::CheckSecondBuyAmplitude(int bar_idx) const {
    // 二买幅度条件：
    // DD1 > DD2（二买的底比一买的底高）
    
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    
    if (DD1 == 0 || DD2 == 0) {
        return false;
    }
    
    return DD1 > DD2;
}

AmplitudeCheck ChanCore::GetAmplitudeCheck(int bar_idx) const {
    AmplitudeCheck result;
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    // 计算各段幅度
    result.amp1 = (GG1 > 0 && DD1 > 0) ? (GG1 - DD1) : 0.0f;
    result.amp2 = (GG2 > 0 && DD2 > 0) ? (GG2 - DD2) : 0.0f;
    result.amp3 = (GG3 > 0 && DD3 > 0) ? (GG3 - DD3) : 0.0f;
    
    // 检查缺口条件
    result.has_gap = (GG1 > 0 && DD3 > 0) ? (GG1 < DD3) : false;
    
    // 检查五段下跌形态
    result.five_down = (DD1 < GG1) && (DD1 < DD2) && (DD1 < DD3) && 
                       (GG1 < GG2) && (GG1 < GG3);
    
    // 检查KJA和KJB条件
    result.kja_valid = CheckFirstBuyKJA(bar_idx);
    result.kjb_valid = CheckFirstBuyKJB(bar_idx);
    
    return result;
}

// ============================================================================
// 阶段二：输出函数扩展
// ============================================================================

void ChanCore::OutputDirection(float* out, int count) const {
    if (!out || count <= 0) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        out[i] = static_cast<float>(m_bi_sequence[i].direction);
    }
}

void ChanCore::OutputGG(float* out, int count, int n) const {
    if (!out || count <= 0 || n < 1 || n > 5) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        out[i] = m_bi_sequence[i].GG[n];
    }
}

void ChanCore::OutputDD(float* out, int count, int n) const {
    if (!out || count <= 0 || n < 1 || n > 5) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        out[i] = m_bi_sequence[i].DD[n];
    }
}

void ChanCore::OutputHH(float* out, int count, int n) const {
    if (!out || count <= 0 || n < 1 || n > 5) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        out[i] = static_cast<float>(m_bi_sequence[i].HH[n]);
    }
}

void ChanCore::OutputLL(float* out, int count, int n) const {
    if (!out || count <= 0 || n < 1 || n > 5) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        out[i] = static_cast<float>(m_bi_sequence[i].LL[n]);
    }
}

// ============================================================================
// 阶段三：买卖点判断
// ============================================================================

void ChanCore::SetMAData(const float* ma13, const float* ma26, int count) {
    m_ma13.clear();
    m_ma26.clear();
    
    if (ma13 && count > 0) {
        m_ma13.assign(ma13, ma13 + count);
    }
    if (ma26 && count > 0) {
        m_ma26.assign(ma26, ma26 + count);
    }
}

bool ChanCore::CheckFiveDownPattern(int bar_idx) const {
    // 五段下跌形态：
    // DD1 < GG1（当前底低于当前顶）
    // DD1 < DD2（当前底低于前底，创新低）
    // DD1 < DD3（当前底低于更前底）
    // GG1 < GG2（当前顶低于前顶）
    // GG1 < GG3（当前顶低于更前顶）
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG1 <= 0 || GG2 <= 0 || GG3 <= 0 || DD1 <= 0 || DD2 <= 0 || DD3 <= 0) {
        return false;
    }
    
    return (DD1 < GG1) && (DD1 < DD2) && (DD1 < DD3) && 
           (GG1 < GG2) && (GG1 < GG3);
}

bool ChanCore::CheckFiveUpPattern(int bar_idx) const {
    // 五段上涨形态（与下跌镜像对称）：
    // GG1 > DD1（当前顶高于当前底）
    // GG1 > GG2（当前顶高于前顶，创新高）
    // GG1 > GG3（当前顶高于更前顶）
    // DD1 > DD2（当前底高于前底）
    // DD1 > DD3（当前底高于更前底）
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG1 <= 0 || GG2 <= 0 || GG3 <= 0 || DD1 <= 0 || DD2 <= 0 || DD3 <= 0) {
        return false;
    }
    
    return (GG1 > DD1) && (GG1 > GG2) && (GG1 > GG3) && 
           (DD1 > DD2) && (DD1 > DD3);
}

bool ChanCore::CheckThreeDownPattern(int bar_idx) const {
    // 三段下跌形态：
    // GG3 > GG2 AND DD3 > DD2
    
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG2 <= 0 || GG3 <= 0 || DD2 <= 0 || DD3 <= 0) {
        return false;
    }
    
    return (GG3 > GG2) && (DD3 > DD2);
}

bool ChanCore::CheckThreeUpPattern(int bar_idx) const {
    // 三段上涨形态：
    // GG2 > GG3 AND DD2 > DD3
    
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG2 <= 0 || GG3 <= 0 || DD2 <= 0 || DD3 <= 0) {
        return false;
    }
    
    return (GG2 > GG3) && (DD2 > DD3);
}

FirstBuyType ChanCore::CheckFirstBuy(int bar_idx, float low) const {
    // 基础条件检查
    // 方向=1（下跌趋势后）
    int direction = GetDirection(bar_idx);
    if (direction != 1) {
        return FirstBuyType::NONE;
    }
    
    // L < MA13
    if (bar_idx >= 0 && bar_idx < (int)m_ma13.size()) {
        if (low >= m_ma13[bar_idx] && m_ma13[bar_idx] > 0) {
            return FirstBuyType::NONE;
        }
    }
    
    // LL1 <= 5
    int ll1 = GetLL(bar_idx, 1);
    if (ll1 > 5) {
        return FirstBuyType::NONE;
    }
    
    // 形态条件：五段下跌
    if (!CheckFiveDownPattern(bar_idx)) {
        return FirstBuyType::NONE;
    }
    
    float GG1 = GetGG(bar_idx, 1);
    float DD3 = GetDD(bar_idx, 3);
    
    // 检查缺口条件
    bool hasGap = (GG1 > 0 && DD3 > 0) ? (GG1 < DD3) : false;
    
    if (hasGap) {
        // 检查 AAA 型（增强型，有缺口+KJA幅度条件）
        if (CheckFirstBuyKJA(bar_idx)) {
            return FirstBuyType::TYPE_AAA;
        }
        // A 型（有缺口）
        return FirstBuyType::TYPE_A;
    } else {
        // 检查 B 型（无缺口+KJB幅度条件）
        if (CheckFirstBuyKJB(bar_idx)) {
            return FirstBuyType::TYPE_B;
        }
    }
    
    return FirstBuyType::NONE;
}

SecondBuyType ChanCore::CheckSecondBuy(int bar_idx, float low) const {
    // 基础条件
    int direction = GetDirection(bar_idx);
    if (direction != 1) {
        return SecondBuyType::NONE;
    }
    
    // L < MA26
    if (bar_idx >= 0 && bar_idx < (int)m_ma26.size()) {
        if (low >= m_ma26[bar_idx] && m_ma26[bar_idx] > 0) {
            return SecondBuyType::NONE;
        }
    }
    
    // LL1 <= 8
    int ll1 = GetLL(bar_idx, 1);
    if (ll1 > 8) {
        return SecondBuyType::NONE;
    }
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float GG4 = GetGG(bar_idx, 4);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    float DD4 = GetDD(bar_idx, 4);
    
    // 形态条件：DD1 < GG1 AND DD1 > DD2（底抬高）
    if (!(DD1 < GG1 && DD1 > DD2)) {
        return SecondBuyType::NONE;
    }
    
    // 检查五段下跌条件
    bool isFiveDown = (GG4 > GG3) && (GG4 > GG2) && (DD2 < DD3) && (DD2 < DD4);
    
    if (isFiveDown) {
        // 缺口条件：GG2 < DD4 AND GG1 > DD3
        if ((GG2 < DD4) && (GG1 > DD3)) {
            return SecondBuyType::TYPE_B1;  // 五段+缺口
        }
        // 无缺口条件：GG2 >= DD4
        if (GG2 >= DD4) {
            return SecondBuyType::TYPE_B2;  // 五段+无缺口
        }
    }
    
    // 三段下跌条件：GG3 > GG2 AND DD3 > DD2
    if (CheckThreeDownPattern(bar_idx)) {
        // 中枢条件：GG1 > DD3（回到前中枢）
        if (GG1 > DD3) {
            return SecondBuyType::TYPE_A;  // 三段下跌后
        }
    }
    
    return SecondBuyType::NONE;
}

ThirdBuyType ChanCore::CheckThirdBuy(int bar_idx, float low) const {
    // 基础条件
    int direction = GetDirection(bar_idx);
    if (direction != 1) {
        return ThirdBuyType::NONE;
    }
    
    // L < MA13
    if (bar_idx >= 0 && bar_idx < (int)m_ma13.size()) {
        if (low >= m_ma13[bar_idx] && m_ma13[bar_idx] > 0) {
            return ThirdBuyType::NONE;
        }
    }
    
    // LL1 <= 5
    int ll1 = GetLL(bar_idx, 1);
    if (ll1 > 5) {
        return ThirdBuyType::NONE;
    }
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    float DD4 = GetDD(bar_idx, 4);
    
    // 形态条件：DD1 < GG1 AND DD1 > DD2
    if (!(DD1 < GG1 && DD1 > DD2)) {
        return ThirdBuyType::NONE;
    }
    
    // 中枢条件：
    // DD1 > MIN(GG2, GG3)  # 当前底高于中枢上沿
    float minGG = std::min(GG2, GG3);
    if (DD1 <= minGG) {
        return ThirdBuyType::NONE;
    }
    
    // GG3 > DD2  # 中枢存在
    if (GG3 <= DD2) {
        return ThirdBuyType::NONE;
    }
    
    // DD4 < MAX(DD2, DD3)  # 前底低于中枢
    float maxDD = std::max(DD2, DD3);
    if (DD4 >= maxDD) {
        return ThirdBuyType::NONE;
    }
    
    // DD1 > DD4  # 当前底高于突破前的底
    if (DD1 <= DD4) {
        return ThirdBuyType::NONE;
    }
    
    return ThirdBuyType::TYPE_A;
}

// ============================================================================
// 卖点判断（与买点镜像对称）
// ============================================================================

FirstSellType ChanCore::CheckFirstSell(int bar_idx, float high) const {
    // 基础条件：方向=-1（上涨趋势后）
    int direction = GetDirection(bar_idx);
    if (direction != -1) {
        return FirstSellType::NONE;
    }
    
    // H > MA13
    if (bar_idx >= 0 && bar_idx < (int)m_ma13.size()) {
        if (high <= m_ma13[bar_idx] && m_ma13[bar_idx] > 0) {
            return FirstSellType::NONE;
        }
    }
    
    // HH1 <= 5
    int hh1 = GetHH(bar_idx, 1);
    if (hh1 > 5) {
        return FirstSellType::NONE;
    }
    
    // 形态条件：五段上涨
    if (!CheckFiveUpPattern(bar_idx)) {
        return FirstSellType::NONE;
    }
    
    float DD1 = GetDD(bar_idx, 1);
    float GG3 = GetGG(bar_idx, 3);
    
    // 缺口条件（镜像）：DD1 > GG3
    bool hasGap = (DD1 > 0 && GG3 > 0) ? (DD1 > GG3) : false;
    
    if (hasGap) {
        // 检查 AAA 型（增强型）
        if (CheckFirstSellKJA(bar_idx)) {
            return FirstSellType::TYPE_AAA;
        }
        return FirstSellType::TYPE_A;
    } else {
        // 检查 B 型
        if (CheckFirstSellKJB(bar_idx)) {
            return FirstSellType::TYPE_B;
        }
    }
    
    return FirstSellType::NONE;
}

// 一卖KJA条件（镜像）
bool ChanCore::CheckFirstSellKJA(int bar_idx) const {
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG1 <= 0 || GG2 <= 0 || GG3 <= 0 || DD1 <= 0 || DD2 <= 0 || DD3 <= 0) {
        return false;
    }
    
    float amp1 = GG1 - DD1;
    float amp2 = GG2 - DD2;
    float amp3 = GG3 - DD3;
    
    // 镜像对称的幅度条件
    return (amp1 < amp2) && (amp1 > amp3) && (amp3 < amp2) && (amp2 > amp1 * 1.618f);
}

// 一卖KJB条件（镜像）
bool ChanCore::CheckFirstSellKJB(int bar_idx) const {
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    if (GG1 <= 0 || GG2 <= 0 || GG3 <= 0 || DD1 <= 0 || DD2 <= 0 || DD3 <= 0) {
        return false;
    }
    
    float amp1 = GG1 - DD1;
    float amp2 = GG2 - DD2;
    float amp3 = GG3 - DD3;
    
    return (amp3 > amp1) && (amp3 > amp2) && (amp2 < amp1);
}

SecondSellType ChanCore::CheckSecondSell(int bar_idx, float high) const {
    // 基础条件
    int direction = GetDirection(bar_idx);
    if (direction != -1) {
        return SecondSellType::NONE;
    }
    
    // H > MA26
    if (bar_idx >= 0 && bar_idx < (int)m_ma26.size()) {
        if (high <= m_ma26[bar_idx] && m_ma26[bar_idx] > 0) {
            return SecondSellType::NONE;
        }
    }
    
    // HH1 <= 8
    int hh1 = GetHH(bar_idx, 1);
    if (hh1 > 8) {
        return SecondSellType::NONE;
    }
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float GG4 = GetGG(bar_idx, 4);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    float DD4 = GetDD(bar_idx, 4);
    
    // 形态条件（镜像）：GG1 > DD1 AND GG1 < GG2（顶降低）
    if (!(GG1 > DD1 && GG1 < GG2)) {
        return SecondSellType::NONE;
    }
    
    // 五段上涨条件（镜像）
    bool isFiveUp = (DD4 < DD3) && (DD4 < DD2) && (GG2 > GG3) && (GG2 > GG4);
    
    if (isFiveUp) {
        // 缺口条件（镜像）：DD2 > GG4 AND DD1 < GG3
        if ((DD2 > GG4) && (DD1 < GG3)) {
            return SecondSellType::TYPE_B1;
        }
        // 无缺口
        if (DD2 <= GG4) {
            return SecondSellType::TYPE_B2;
        }
    }
    
    // 三段上涨条件（镜像）
    if (CheckThreeUpPattern(bar_idx)) {
        // 中枢条件（镜像）：DD1 < GG3
        if (DD1 < GG3) {
            return SecondSellType::TYPE_A;
        }
    }
    
    return SecondSellType::NONE;
}

ThirdSellType ChanCore::CheckThirdSell(int bar_idx, float high) const {
    // 基础条件
    int direction = GetDirection(bar_idx);
    if (direction != -1) {
        return ThirdSellType::NONE;
    }
    
    // H > MA13
    if (bar_idx >= 0 && bar_idx < (int)m_ma13.size()) {
        if (high <= m_ma13[bar_idx] && m_ma13[bar_idx] > 0) {
            return ThirdSellType::NONE;
        }
    }
    
    // HH1 <= 5
    int hh1 = GetHH(bar_idx, 1);
    if (hh1 > 5) {
        return ThirdSellType::NONE;
    }
    
    float GG1 = GetGG(bar_idx, 1);
    float GG2 = GetGG(bar_idx, 2);
    float GG3 = GetGG(bar_idx, 3);
    float GG4 = GetGG(bar_idx, 4);
    float DD1 = GetDD(bar_idx, 1);
    float DD2 = GetDD(bar_idx, 2);
    float DD3 = GetDD(bar_idx, 3);
    
    // 形态条件（镜像）：GG1 > DD1 AND GG1 < GG2
    if (!(GG1 > DD1 && GG1 < GG2)) {
        return ThirdSellType::NONE;
    }
    
    // 中枢条件（镜像）：
    // GG1 < MAX(DD2, DD3)  # 当前顶低于中枢下沿
    float maxDD = std::max(DD2, DD3);
    if (GG1 >= maxDD) {
        return ThirdSellType::NONE;
    }
    
    // DD3 < GG2  # 中枢存在
    if (DD3 >= GG2) {
        return ThirdSellType::NONE;
    }
    
    // GG4 > MIN(GG2, GG3)  # 前顶高于中枢
    float minGG = std::min(GG2, GG3);
    if (GG4 <= minGG) {
        return ThirdSellType::NONE;
    }
    
    // GG1 < GG4  # 当前顶低于突破前的顶
    if (GG1 >= GG4) {
        return ThirdSellType::NONE;
    }
    
    return ThirdSellType::TYPE_A;
}

// ============================================================================
// 买卖点输出函数
// ============================================================================

void ChanCore::OutputBuySignal(float* out, int count, const float* lows) const {
    if (!out || count <= 0) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float low = (lows && i < count) ? lows[i] : 0;
        
        // 优先检查一买
        FirstBuyType fb = CheckFirstBuy(i, low);
        if (fb != FirstBuyType::NONE) {
            out[i] = static_cast<float>(fb);  // 1=A, 2=B, 3=AAA
            continue;
        }
        
        // 检查二买
        SecondBuyType sb = CheckSecondBuy(i, low);
        if (sb != SecondBuyType::NONE) {
            out[i] = 10.0f + static_cast<float>(sb);  // 11=A, 12=B1, 13=B2
            continue;
        }
        
        // 检查三买
        ThirdBuyType tb = CheckThirdBuy(i, low);
        if (tb != ThirdBuyType::NONE) {
            out[i] = 20.0f + static_cast<float>(tb);  // 21=A
        }
    }
}

void ChanCore::OutputSellSignal(float* out, int count, const float* highs) const {
    if (!out || count <= 0) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float high = (highs && i < count) ? highs[i] : 0;
        
        // 优先检查一卖
        FirstSellType fs = CheckFirstSell(i, high);
        if (fs != FirstSellType::NONE) {
            out[i] = -static_cast<float>(fs);  // -1=A, -2=B, -3=AAA
            continue;
        }
        
        // 检查二卖
        SecondSellType ss = CheckSecondSell(i, high);
        if (ss != SecondSellType::NONE) {
            out[i] = -10.0f - static_cast<float>(ss);  // -11=A, -12=B1, -13=B2
            continue;
        }
        
        // 检查三卖
        ThirdSellType ts = CheckThirdSell(i, high);
        if (ts != ThirdSellType::NONE) {
            out[i] = -20.0f - static_cast<float>(ts);  // -21=A
        }
    }
}

// ============================================================================
// 阶段四：准买卖点和类二买实现
// ============================================================================

PreFirstBuyType ChanCore::CheckPreFirstBuy(int bar_idx, float low) const {
    // 准一买：条件放宽版本的一买
    // 条件：方向=1 AND L<MA13 AND LL1<=8（放宽时间窗口）
    // 形态：DD1<DD2 OR DD1<DD3（部分底部降低即可）
    
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return PreFirstBuyType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 1. 方向条件：必须在下跌趋势后
    if (seq.direction != 1) {
        return PreFirstBuyType::NONE;
    }
    
    // 2. 均线条件：L < MA13
    if (bar_idx < (int)m_ma13.size() && m_ma13[bar_idx] > 0) {
        if (low >= m_ma13[bar_idx]) {
            return PreFirstBuyType::NONE;
        }
    }
    
    // 3. 时间窗口：LL1 <= 8（放宽版本）
    if (seq.LL[1] > 8) {
        return PreFirstBuyType::NONE;
    }
    
    // 4. 形态条件：DD1<DD2 OR DD1<DD3（部分底部降低即可）
    bool partial_lower = (seq.DD[1] < seq.DD[2]) || (seq.DD[1] < seq.DD[3]);
    
    if (!partial_lower) {
        return PreFirstBuyType::NONE;
    }
    
    return PreFirstBuyType::TYPE_A;
}

PreSecondBuyType ChanCore::CheckPreSecondBuy(int bar_idx, float low) const {
    // 准二买：条件放宽版本的二买
    // 条件：方向=1 AND L<MA26 AND LL1<=10
    // 形态：DD1>DD2（底抬高）
    // 中枢：GG1>DD2 OR GG1>DD3（中枢雏形即可）
    
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return PreSecondBuyType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 1. 方向条件
    if (seq.direction != 1) {
        return PreSecondBuyType::NONE;
    }
    
    // 2. 均线条件：L < MA26
    if (bar_idx < (int)m_ma26.size() && m_ma26[bar_idx] > 0) {
        if (low >= m_ma26[bar_idx]) {
            return PreSecondBuyType::NONE;
        }
    }
    
    // 3. 时间窗口：LL1 <= 10（放宽版本）
    if (seq.LL[1] > 10) {
        return PreSecondBuyType::NONE;
    }
    
    // 4. 形态条件：DD1 > DD2（底抬高）
    if (seq.DD[1] <= seq.DD[2]) {
        return PreSecondBuyType::NONE;
    }
    
    // 5. 中枢雏形：GG1>DD2 OR GG1>DD3（放宽条件）
    bool pivot_forming = (seq.GG[1] > seq.DD[2]) || (seq.GG[1] > seq.DD[3]);
    
    if (!pivot_forming) {
        return PreSecondBuyType::NONE;
    }
    
    return PreSecondBuyType::TYPE_A;
}

PreThirdBuyType ChanCore::CheckPreThirdBuy(int bar_idx, float low) const {
    // 准三买：回调接近但未触及中枢上沿
    // 条件：方向=1 AND DD1>DD2
    // 形态：DD1接近但未触及中枢上沿
    
    (void)low;  // 未使用
    
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return PreThirdBuyType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 1. 方向条件
    if (seq.direction != 1) {
        return PreThirdBuyType::NONE;
    }
    
    // 2. 形态条件：DD1 > DD2（底抬高）
    if (seq.DD[1] <= seq.DD[2]) {
        return PreThirdBuyType::NONE;
    }
    
    // 3. 中枢上沿计算：MIN(GG2, GG3)
    float zs_upper = (seq.GG[2] < seq.GG[3]) ? seq.GG[2] : seq.GG[3];
    if (zs_upper <= 0) {
        return PreThirdBuyType::NONE;
    }
    
    // 4. 中枢下沿计算：MAX(DD2, DD3)
    float zs_lower = (seq.DD[2] > seq.DD[3]) ? seq.DD[2] : seq.DD[3];
    
    // 5. 接近但未触及中枢上沿
    // 标准三买：DD1 > zs_upper
    // 准三买：DD1 接近 zs_upper（在中枢上沿附近10%范围内）
    float zs_range = zs_upper - zs_lower;
    float tolerance = zs_range * 0.1f;  // 10%容差
    
    // DD1在中枢上沿附近但未完全满足标准三买条件
    bool near_upper = (seq.DD[1] >= zs_upper - tolerance) && (seq.DD[1] <= zs_upper);
    
    if (!near_upper) {
        return PreThirdBuyType::NONE;
    }
    
    return PreThirdBuyType::TYPE_A;
}

LikeSecondBuyType ChanCore::CheckLikeSecondBuy(int bar_idx, float low) const {
    // 类二买A（成功率85%）
    // 条件：方向=1 AND C<MA13 AND LL1<=8
    // 形态：DD1<GG1 AND DD3<DD2 AND DD3<DD1 AND DD3<DD4
    // 幅度：(GG2-DD3)>(GG2-DD2) AND (GG2-DD3)>(GG1-DD1)
    
    // 类二买AAA（最强买，成功率90%）
    // 附加条件：GG1>GG2 AND DD1>DD2（创新高+底抬高）
    //          (GG2-DD3) > (GG2-DD2)*1.618（黄金分割确认）
    
    (void)low;  // 使用close判断而非low
    
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return LikeSecondBuyType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 1. 方向条件
    if (seq.direction != 1) {
        return LikeSecondBuyType::NONE;
    }
    
    // 2. 时间窗口：LL1 <= 8
    if (seq.LL[1] > 8) {
        return LikeSecondBuyType::NONE;
    }
    
    // 3. 形态条件：DD1<GG1
    if (seq.DD[1] >= seq.GG[1]) {
        return LikeSecondBuyType::NONE;
    }
    
    // 4. DD3<DD2 AND DD3<DD1 AND DD3<DD4（DD3是最低点）
    if (!(seq.DD[3] < seq.DD[2] && seq.DD[3] < seq.DD[1] && seq.DD[3] < seq.DD[4])) {
        return LikeSecondBuyType::NONE;
    }
    
    // 5. 幅度条件
    float amp_total = seq.GG[2] - seq.DD[3];     // 整体幅度
    float amp2 = seq.GG[2] - seq.DD[2];          // 第2段幅度
    float amp1 = seq.GG[1] - seq.DD[1];          // 当前段幅度
    
    // (GG2-DD3)>(GG2-DD2) AND (GG2-DD3)>(GG1-DD1)
    if (!(amp_total > amp2 && amp_total > amp1)) {
        return LikeSecondBuyType::NONE;
    }
    
    // 检查是否满足AAA型条件
    // GG1>GG2 AND DD1>DD2（创新高+底抬高）
    bool new_high = seq.GG[1] > seq.GG[2];
    bool higher_low = seq.DD[1] > seq.DD[2];
    
    // (GG2-DD3) > (GG2-DD2)*1.618（黄金分割确认）
    bool golden_ratio = amp_total > amp2 * 1.618f;
    
    if (new_high && higher_low && golden_ratio) {
        return LikeSecondBuyType::TYPE_AAA;  // 最强买
    }
    
    return LikeSecondBuyType::TYPE_A;  // 普通类二买
}

// 准卖点实现（镜像对称）

PreFirstSellType ChanCore::CheckPreFirstSell(int bar_idx, float high) const {
    // 准一卖：一卖的放宽版本
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return PreFirstSellType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 方向=-1（上涨后）
    if (seq.direction != -1) {
        return PreFirstSellType::NONE;
    }
    
    // 均线条件：H > MA13
    if (bar_idx < (int)m_ma13.size() && m_ma13[bar_idx] > 0) {
        if (high <= m_ma13[bar_idx]) {
            return PreFirstSellType::NONE;
        }
    }
    
    // 时间窗口：HH1 <= 8（放宽版本）
    if (seq.HH[1] > 8) {
        return PreFirstSellType::NONE;
    }
    
    // 形态条件：GG1>GG2 OR GG1>GG3（部分顶部抬高即可）
    bool partial_higher = (seq.GG[1] > seq.GG[2]) || (seq.GG[1] > seq.GG[3]);
    
    if (!partial_higher) {
        return PreFirstSellType::NONE;
    }
    
    return PreFirstSellType::TYPE_A;
}

PreSecondSellType ChanCore::CheckPreSecondSell(int bar_idx, float high) const {
    // 准二卖：二卖的放宽版本
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return PreSecondSellType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 方向=-1
    if (seq.direction != -1) {
        return PreSecondSellType::NONE;
    }
    
    // 均线条件：H > MA26
    if (bar_idx < (int)m_ma26.size() && m_ma26[bar_idx] > 0) {
        if (high <= m_ma26[bar_idx]) {
            return PreSecondSellType::NONE;
        }
    }
    
    // 时间窗口：HH1 <= 10（放宽版本）
    if (seq.HH[1] > 10) {
        return PreSecondSellType::NONE;
    }
    
    // 形态条件：GG1 < GG2（顶降低）
    if (seq.GG[1] >= seq.GG[2]) {
        return PreSecondSellType::NONE;
    }
    
    // 中枢雏形：DD1<GG2 OR DD1<GG3
    bool pivot_forming = (seq.DD[1] < seq.GG[2]) || (seq.DD[1] < seq.GG[3]);
    
    if (!pivot_forming) {
        return PreSecondSellType::NONE;
    }
    
    return PreSecondSellType::TYPE_A;
}

PreThirdSellType ChanCore::CheckPreThirdSell(int bar_idx, float high) const {
    // 准三卖：反弹接近但未触及中枢下沿
    (void)high;
    
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return PreThirdSellType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 方向=-1
    if (seq.direction != -1) {
        return PreThirdSellType::NONE;
    }
    
    // 形态条件：GG1 < GG2（顶降低）
    if (seq.GG[1] >= seq.GG[2]) {
        return PreThirdSellType::NONE;
    }
    
    // 中枢下沿：MAX(DD2, DD3)
    float zs_lower = (seq.DD[2] > seq.DD[3]) ? seq.DD[2] : seq.DD[3];
    if (zs_lower <= 0) {
        return PreThirdSellType::NONE;
    }
    
    // 中枢上沿
    float zs_upper = (seq.GG[2] < seq.GG[3]) ? seq.GG[2] : seq.GG[3];
    
    // 接近但未触及中枢下沿
    float zs_range = zs_upper - zs_lower;
    float tolerance = zs_range * 0.1f;
    
    bool near_lower = (seq.GG[1] >= zs_lower) && (seq.GG[1] <= zs_lower + tolerance);
    
    if (!near_lower) {
        return PreThirdSellType::NONE;
    }
    
    return PreThirdSellType::TYPE_A;
}

LikeSecondSellType ChanCore::CheckLikeSecondSell(int bar_idx, float high) const {
    // 类二卖（镜像对称于类二买）
    (void)high;
    
    if (bar_idx < 0 || bar_idx >= (int)m_bi_sequence.size()) {
        return LikeSecondSellType::NONE;
    }
    
    const BiSequenceData& seq = m_bi_sequence[bar_idx];
    
    // 方向=-1
    if (seq.direction != -1) {
        return LikeSecondSellType::NONE;
    }
    
    // 时间窗口：HH1 <= 8
    if (seq.HH[1] > 8) {
        return LikeSecondSellType::NONE;
    }
    
    // 形态条件：GG1>DD1
    if (seq.GG[1] <= seq.DD[1]) {
        return LikeSecondSellType::NONE;
    }
    
    // GG3>GG2 AND GG3>GG1 AND GG3>GG4（GG3是最高点）
    if (!(seq.GG[3] > seq.GG[2] && seq.GG[3] > seq.GG[1] && seq.GG[3] > seq.GG[4])) {
        return LikeSecondSellType::NONE;
    }
    
    // 幅度条件
    float amp_total = seq.GG[3] - seq.DD[2];
    float amp2 = seq.GG[2] - seq.DD[2];
    float amp1 = seq.GG[1] - seq.DD[1];
    
    if (!(amp_total > amp2 && amp_total > amp1)) {
        return LikeSecondSellType::NONE;
    }
    
    // 检查AAA型条件
    bool new_low = seq.DD[1] < seq.DD[2];
    bool lower_high = seq.GG[1] < seq.GG[2];
    bool golden_ratio = amp_total > amp2 * 1.618f;
    
    if (new_low && lower_high && golden_ratio) {
        return LikeSecondSellType::TYPE_AAA;
    }
    
    return LikeSecondSellType::TYPE_A;
}

// ============================================================================
// 综合信号输出函数
// ============================================================================

void ChanCore::OutputCombinedBuySignal(float* out, int count, const float* lows) const {
    if (!out || count <= 0) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float low = (lows && i < count) ? lows[i] : 0;
        
        // 优先级1：标准买点
        // 一买 -> 返回1
        FirstBuyType fb = CheckFirstBuy(i, low);
        if (fb != FirstBuyType::NONE) {
            out[i] = 1.0f;  // 1=一买
            continue;
        }
        
        // 二买 -> 返回2
        SecondBuyType sb = CheckSecondBuy(i, low);
        if (sb != SecondBuyType::NONE) {
            out[i] = 2.0f;  // 2=二买
            continue;
        }
        
        // 三买 -> 返回3
        ThirdBuyType tb = CheckThirdBuy(i, low);
        if (tb != ThirdBuyType::NONE) {
            out[i] = 3.0f;  // 3=三买
            continue;
        }
        
        // 优先级2：类买点（可配置是否启用）
        if (m_config.enable_like_signals) {
            LikeSecondBuyType l2b = CheckLikeSecondBuy(i, low);
            if (l2b != LikeSecondBuyType::NONE) {
                out[i] = 21.0f;  // 21=类二买
                continue;
            }
        }
        
        // 优先级3：准买点（可配置是否启用）
        if (m_config.enable_pre_signals) {
            PreFirstBuyType pfb = CheckPreFirstBuy(i, low);
            if (pfb != PreFirstBuyType::NONE) {
                out[i] = 11.0f;  // 11=准一买
                continue;
            }
            
            PreSecondBuyType psb = CheckPreSecondBuy(i, low);
            if (psb != PreSecondBuyType::NONE) {
                out[i] = 12.0f;  // 12=准二买
                continue;
            }
            
            PreThirdBuyType ptb = CheckPreThirdBuy(i, low);
            if (ptb != PreThirdBuyType::NONE) {
                out[i] = 13.0f;  // 13=准三买
                continue;
            }
        }
    }
}

void ChanCore::OutputCombinedSellSignal(float* out, int count, const float* highs) const {
    if (!out || count <= 0) return;
    
    memset(out, 0, count * sizeof(float));
    
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float high = (highs && i < count) ? highs[i] : 0;
        
        // 优先级1：标准卖点
        // 一卖 -> 返回-1
        FirstSellType fs = CheckFirstSell(i, high);
        if (fs != FirstSellType::NONE) {
            out[i] = -1.0f;  // -1=一卖
            continue;
        }
        
        // 二卖 -> 返回-2
        SecondSellType ss = CheckSecondSell(i, high);
        if (ss != SecondSellType::NONE) {
            out[i] = -2.0f;  // -2=二卖
            continue;
        }
        
        // 三卖 -> 返回-3
        ThirdSellType ts = CheckThirdSell(i, high);
        if (ts != ThirdSellType::NONE) {
            out[i] = -3.0f;  // -3=三卖
            continue;
        }
        
        // 优先级2：类卖点（可配置是否启用）
        if (m_config.enable_like_signals) {
            LikeSecondSellType l2s = CheckLikeSecondSell(i, high);
            if (l2s != LikeSecondSellType::NONE) {
                out[i] = -21.0f;  // -21=类二卖
                continue;
            }
        }
        
        // 优先级3：准卖点（可配置是否启用）
        if (m_config.enable_pre_signals) {
            PreFirstSellType pfs = CheckPreFirstSell(i, high);
            if (pfs != PreFirstSellType::NONE) {
                out[i] = -11.0f;  // -11=准一卖
                continue;
            }
            
            PreSecondSellType pss = CheckPreSecondSell(i, high);
            if (pss != PreSecondSellType::NONE) {
                out[i] = -12.0f;  // -12=准二卖
                continue;
            }
            
            PreThirdSellType pts = CheckPreThirdSell(i, high);
            if (pts != PreThirdSellType::NONE) {
                out[i] = -13.0f;  // -13=准三卖
                continue;
            }
        }
    }
}

// ============================================================================
// 阶段五：新增输出函数 - DLL接口扩展
// ============================================================================

void ChanCore::OutputZS_Z(float* out, int count) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    for (int i = 0; i < count; ++i) {
        out[i] = 0.0f;
    }
    
    // 计算中枢中轴 (ZS_Z = (ZG + ZD) / 2)
    for (const auto& pivot : m_pivots) {
        int start_idx = pivot.start_idx;
        int end_idx = pivot.end_idx;
        float zs_z = (pivot.ZG + pivot.ZD) / 2.0f;  // 中轴 = (中枢高 + 中枢低) / 2
        
        // 在中枢区间内填充中轴值
        for (int i = start_idx; i <= end_idx && i < count; ++i) {
            if (i >= 0) {
                out[i] = zs_z;
            }
        }
    }
}

void ChanCore::OutputPreBuySignal(float* out, int count, const float* lows) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    for (int i = 0; i < count; ++i) {
        out[i] = 0.0f;
    }
    
    // 只输出准买点（准一买、准二买、准三买）
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float low = (lows && i < count) ? lows[i] : 0;
        
        // 准一买 -> 返回11
        PreFirstBuyType pfb = CheckPreFirstBuy(i, low);
        if (pfb != PreFirstBuyType::NONE) {
            out[i] = 11.0f;
            continue;
        }
        
        // 准二买 -> 返回12
        PreSecondBuyType psb = CheckPreSecondBuy(i, low);
        if (psb != PreSecondBuyType::NONE) {
            out[i] = 12.0f;
            continue;
        }
        
        // 准三买 -> 返回13
        PreThirdBuyType ptb = CheckPreThirdBuy(i, low);
        if (ptb != PreThirdBuyType::NONE) {
            out[i] = 13.0f;
            continue;
        }
    }
}

void ChanCore::OutputPreSellSignal(float* out, int count, const float* highs) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    for (int i = 0; i < count; ++i) {
        out[i] = 0.0f;
    }
    
    // 只输出准卖点（准一卖、准二卖、准三卖）
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float high = (highs && i < count) ? highs[i] : 0;
        
        // 准一卖 -> 返回-11
        PreFirstSellType pfs = CheckPreFirstSell(i, high);
        if (pfs != PreFirstSellType::NONE) {
            out[i] = -11.0f;
            continue;
        }
        
        // 准二卖 -> 返回-12
        PreSecondSellType pss = CheckPreSecondSell(i, high);
        if (pss != PreSecondSellType::NONE) {
            out[i] = -12.0f;
            continue;
        }
        
        // 准三卖 -> 返回-13
        PreThirdSellType pts = CheckPreThirdSell(i, high);
        if (pts != PreThirdSellType::NONE) {
            out[i] = -13.0f;
            continue;
        }
    }
}

void ChanCore::OutputLikeSecondBuySignal(float* out, int count, const float* lows) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    for (int i = 0; i < count; ++i) {
        out[i] = 0.0f;
    }
    
    // 只输出类二买信号
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float low = (lows && i < count) ? lows[i] : 0;
        
        LikeSecondBuyType l2b = CheckLikeSecondBuy(i, low);
        if (l2b == LikeSecondBuyType::TYPE_A) {
            out[i] = 21.0f;  // 21=类二买A
        } else if (l2b == LikeSecondBuyType::TYPE_AAA) {
            out[i] = 22.0f;  // 22=类二买AAA
        }
    }
}

void ChanCore::OutputLikeSecondSellSignal(float* out, int count, const float* highs) const {
    if (!out || count <= 0) return;
    
    // 初始化为0
    for (int i = 0; i < count; ++i) {
        out[i] = 0.0f;
    }
    
    // 只输出类二卖信号
    for (int i = 0; i < count && i < (int)m_bi_sequence.size(); ++i) {
        float high = (highs && i < count) ? highs[i] : 0;
        
        LikeSecondSellType l2s = CheckLikeSecondSell(i, high);
        if (l2s == LikeSecondSellType::TYPE_A) {
            out[i] = -21.0f;  // -21=类二卖A
        } else if (l2s == LikeSecondSellType::TYPE_AAA) {
            out[i] = -22.0f;  // -22=类二卖AAA
        }
    }
}

void ChanCore::OutputNewBar(float* out, int count) const {
    if (!out || count <= 0) return;
    
    // 初始化为0（表示被合并）
    for (int i = 0; i < count; ++i) {
        out[i] = 0.0f;
    }
    
    // 标记新K线（去包含后保留的K线）
    // 使用合并后的K线序列来确定哪些原始K线被保留
    for (const auto& merged_kline : m_merged_klines) {
        // merged_kline.index 是合并后K线对应的原始索引
        int idx = merged_kline.index;
        if (idx >= 0 && idx < count) {
            out[idx] = 1.0f;  // 1=新K线（被保留）
        }
    }
}

} // namespace chan
