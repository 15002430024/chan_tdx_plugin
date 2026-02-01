#pragma once
// ============================================================================
// 缠论通达信DLL插件 - 核心算法声明
// ============================================================================
// 实现缠论核心算法：去包含、分型识别、笔识别、中枢识别
// 参考文档：《通达信缠论DLL开发需求文档》v2.1
// ============================================================================

#ifndef CHAN_CORE_H
#define CHAN_CORE_H

#include "chan_types.h"
#include <vector>
#include <algorithm>
#include <cmath>

namespace chan {

// ============================================================================
// 配置参数
// ============================================================================

struct ChanConfig {
    int min_bi_len;           // 笔最小K线数量，默认5
    int min_fx_distance;      // 分型最小间隔K线数，默认1
    int min_zs_bi_count;      // 中枢最小笔数量，默认3
    bool strict_bi;           // 严格笔定义（顶底必须有效突破）
    bool enable_like_signals; // 启用类买卖点（类二买等），默认true
    bool enable_pre_signals;  // 启用准买卖点（准一/二/三买等），默认true
    
    ChanConfig() 
        : min_bi_len(5)
        , min_fx_distance(1)
        , min_zs_bi_count(3)
        , strict_bi(true)
        , enable_like_signals(true)
        , enable_pre_signals(true) {}
};

// ============================================================================
// 核心算法类
// ============================================================================

class ChanCore {
public:
    // 构造函数
    ChanCore();
    explicit ChanCore(const ChanConfig& config);
    
    // 配置
    void SetConfig(const ChanConfig& config);
    const ChanConfig& GetConfig() const { return m_config; }
    
    // ========================================================================
    // 主处理流程
    // ========================================================================
    
    /// @brief 完整分析：从原始K线到买卖点
    /// @param highs 最高价数组
    /// @param lows 最低价数组
    /// @param closes 收盘价数组
    /// @param volumes 成交量数组
    /// @param count K线数量
    /// @return 成功返回0，失败返回错误码
    int Analyze(const float* highs, const float* lows, 
                const float* closes, const float* volumes, int count);
    
    // ========================================================================
    // 去包含处理 (5.1)
    // ========================================================================
    
    /// @brief K线去包含处理
    /// @param highs 原始最高价数组
    /// @param lows 原始最低价数组
    /// @param count K线数量
    /// @return 处理后的K线数量
    int RemoveInclude(const float* highs, const float* lows, int count);
    
    /// @brief 获取去包含后的K线
    const std::vector<KLine>& GetMergedKLines() const { return m_merged_klines; }
    
    // ========================================================================
    // 分型识别 (5.2)
    // ========================================================================
    
    /// @brief 分型识别
    /// @return 识别到的分型数量
    int CheckFX();
    
    /// @brief 获取分型列表
    const std::vector<Fractal>& GetFractals() const { return m_fractals; }
    
    // ========================================================================
    // 笔识别 (5.3)
    // ========================================================================
    
    /// @brief 笔识别
    /// @return 识别到的笔数量
    int CheckBI();
    
    /// @brief 获取笔列表
    const std::vector<Stroke>& GetStrokes() const { return m_strokes; }
    
    // ========================================================================
    // 中枢识别 (5.4)
    // ========================================================================
    
    /// @brief 中枢识别
    /// @return 识别到的中枢数量
    int CheckZS();
    
    /// @brief 获取中枢列表
    const std::vector<Pivot>& GetPivots() const { return m_pivots; }
    
    // ========================================================================
    // 递归引用系统 (任务2.2)
    // ========================================================================
    
    /// @brief 构建递归引用序列（GG/DD序列）
    /// @param current_bar_idx 当前K线索引范围（构建从0到current_bar_idx的所有序列）
    void BuildBiSequence(int current_bar_idx);
    
    /// @brief 获取指定K线的GG值
    /// @param kline_idx K线索引
    /// @param n 序号(1-5)
    /// @return GGn的值
    float GetGG(int kline_idx, int n) const;
    
    /// @brief 获取指定K线的DD值
    float GetDD(int kline_idx, int n) const;
    
    /// @brief 获取指定K线到最近顶点的K线数
    int GetHH(int kline_idx, int n) const;
    
    /// @brief 获取指定K线到最近底点的K线数
    int GetLL(int kline_idx, int n) const;
    
    // ========================================================================
    // 方向判断 (任务2.3)
    // ========================================================================
    
    /// @brief 判断指定K线所处的趋势方向
    /// @param kline_idx K线索引
    /// @return 1=下跌趋势后（适合判断买点），-1=上涨趋势后（适合判断卖点），0=震荡
    int GetDirection(int kline_idx) const;
    
    // ========================================================================
    // 幅度计算 (任务2.4)
    // ========================================================================
    
    /// @brief 检查一买KJA条件（有缺口+幅度确认）
    /// @param bar_idx K线索引
    /// @return true=满足条件
    bool CheckFirstBuyKJA(int bar_idx) const;
    
    /// @brief 检查一买KJB条件（无缺口+幅度衰减）
    bool CheckFirstBuyKJB(int bar_idx) const;
    
    /// @brief 检查类二买幅度条件
    bool CheckSecondBuyAmplitude(int bar_idx) const;
    
    /// @brief 获取幅度检查结果
    AmplitudeCheck GetAmplitudeCheck(int bar_idx) const;
    
    // ========================================================================
    // 买卖点判断 (阶段三)
    // ========================================================================
    
    /// @brief 设置均线数据（用于买卖点判断）
    /// @param ma13 MA13数组
    /// @param ma26 MA26数组
    /// @param count 数组长度
    void SetMAData(const float* ma13, const float* ma26, int count);
    
    /// @brief 一买判断
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 一买类型（NONE/A/B/AAA）
    FirstBuyType CheckFirstBuy(int bar_idx, float low) const;
    
    /// @brief 二买判断
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 二买类型（NONE/A/B1/B2）
    SecondBuyType CheckSecondBuy(int bar_idx, float low) const;
    
    /// @brief 三买判断
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 三买类型（NONE/A）
    ThirdBuyType CheckThirdBuy(int bar_idx, float low) const;
    
    /// @brief 一卖判断
    /// @param bar_idx K线索引
    /// @param high 当前K线最高价
    /// @return 一卖类型（NONE/A/B/AAA）
    FirstSellType CheckFirstSell(int bar_idx, float high) const;
    
    /// @brief 二卖判断
    /// @param bar_idx K线索引
    /// @param high 当前K线最高价
    /// @return 二卖类型（NONE/A/B1/B2）
    SecondSellType CheckSecondSell(int bar_idx, float high) const;
    
    /// @brief 三卖判断
    /// @param bar_idx K线索引
    /// @param high 当前K线最高价
    /// @return 三卖类型（NONE/A）
    ThirdSellType CheckThirdSell(int bar_idx, float high) const;
    
    /// @brief 输出买点信号
    /// @param out 输出数组
    /// @param count 数组长度
    /// @param lows 最低价数组（用于买点判断）
    /// @note 输出: 0=无, 1=一买A, 2=一买B, 3=一买AAA, 10=二买A, 11=二买B1, 12=二买B2, 20=三买
    void OutputBuySignal(float* out, int count, const float* lows) const;
    
    /// @brief 输出卖点信号
    /// @param out 输出数组
    /// @param count 数组长度
    /// @param highs 最高价数组（用于卖点判断）
    /// @note 输出: 0=无, -1=一卖A, -2=一卖B, -3=一卖AAA, -10=二卖A, -11=二卖B1, -12=二卖B2, -20=三卖
    void OutputSellSignal(float* out, int count, const float* highs) const;
    
    // ========================================================================
    // 阶段四：准买卖点和类二买判断
    // ========================================================================
    
    /// @brief 准一买判断（条件放宽版）
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 准一买类型
    /// @note 条件：方向=1 AND L<MA13 AND LL1<=8（放宽）
    ///       形态：DD1<DD2 OR DD1<DD3（部分底部降低即可）
    PreFirstBuyType CheckPreFirstBuy(int bar_idx, float low) const;
    
    /// @brief 准二买判断（条件放宽版）
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 准二买类型
    /// @note 条件：方向=1 AND L<MA26 AND LL1<=10
    ///       形态：DD1>DD2（底抬高）
    ///       中枢：GG1>DD2 OR GG1>DD3（中枢雏形即可）
    PreSecondBuyType CheckPreSecondBuy(int bar_idx, float low) const;
    
    /// @brief 准三买判断（条件放宽版）
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 准三买类型
    /// @note 条件：方向=1 AND DD1>DD2
    ///       形态：DD1接近但未触及中枢上沿
    PreThirdBuyType CheckPreThirdBuy(int bar_idx, float low) const;
    
    /// @brief 类二买判断（强二买）
    /// @param bar_idx K线索引
    /// @param low 当前K线最低价
    /// @return 类二买类型（A/AAA）
    /// @note 类二买A: 成功率85%，需满足特殊形态和幅度条件
    ///       类二买AAA: 成功率90%，附加黄金分割确认
    LikeSecondBuyType CheckLikeSecondBuy(int bar_idx, float low) const;
    
    /// @brief 准一卖判断（镜像对称）
    PreFirstSellType CheckPreFirstSell(int bar_idx, float high) const;
    
    /// @brief 准二卖判断（镜像对称）
    PreSecondSellType CheckPreSecondSell(int bar_idx, float high) const;
    
    /// @brief 准三卖判断（镜像对称）
    PreThirdSellType CheckPreThirdSell(int bar_idx, float high) const;
    
    /// @brief 类二卖判断（镜像对称）
    LikeSecondSellType CheckLikeSecondSell(int bar_idx, float high) const;
    
    /// @brief 输出综合买点信号
    /// @param out 输出数组
    /// @param count 数组长度
    /// @param lows 最低价数组
    /// @note 输出: 标准买点(1-30) > 类买点(31-40) > 准买点(41-50)
    ///       1=一买A, 2=一买B, 3=一买AAA
    ///       11=二买A, 12=二买B1, 13=二买B2
    ///       21=三买A
    ///       31=类二买A, 32=类二买AAA
    ///       41=准一买, 42=准二买, 43=准三买
    void OutputCombinedBuySignal(float* out, int count, const float* lows) const;
    
    /// @brief 输出综合卖点信号（镜像对称）
    void OutputCombinedSellSignal(float* out, int count, const float* highs) const;
    
    // ========================================================================
    // 输出函数 - 供通达信接口调用
    // ========================================================================
    
    /// @brief 输出分型标记到数组
    /// @param out 输出数组 (长度=原始K线数量)
    /// @param count 数组长度
    /// @note 输出值: 1=顶分型, -1=底分型, 0=无
    void OutputFX(float* out, int count) const;
    
    /// @brief 输出笔端点到数组
    /// @param out 输出数组
    /// @param count 数组长度
    /// @note 输出值: 端点处为价格，非端点为0
    void OutputBI(float* out, int count) const;
    
    /// @brief 输出中枢高点到数组
    void OutputZS_H(float* out, int count) const;
    
    /// @brief 输出中枢低点到数组
    void OutputZS_L(float* out, int count) const;
    
    /// @brief 输出方向到数组
    /// @note 输出值: 1=下跌后, -1=上涨后, 0=震荡
    void OutputDirection(float* out, int count) const;
    
    /// @brief 输出GG1到数组
    void OutputGG(float* out, int count, int n) const;
    
    /// @brief 输出DD1到数组
    void OutputDD(float* out, int count, int n) const;
    
    /// @brief 输出HH1到数组
    void OutputHH(float* out, int count, int n) const;
    
    /// @brief 输出LL1到数组
    void OutputLL(float* out, int count, int n) const;
    
    // ========================================================================
    // 阶段五新增输出函数
    // ========================================================================
    
    /// @brief 输出中枢中轴到数组
    void OutputZS_Z(float* out, int count) const;
    
    /// @brief 输出准买点信号
    /// @note 输出: 11=准一买, 12=准二买, 13=准三买
    void OutputPreBuySignal(float* out, int count, const float* lows) const;
    
    /// @brief 输出准卖点信号
    /// @note 输出: -11=准一卖, -12=准二卖, -13=准三卖
    void OutputPreSellSignal(float* out, int count, const float* highs) const;
    
    /// @brief 输出类二买信号
    /// @note 输出: 21=类二买A, 22=类二买AAA
    void OutputLikeSecondBuySignal(float* out, int count, const float* lows) const;
    
    /// @brief 输出类二卖信号
    /// @note 输出: -21=类二卖A, -22=类二卖AAA
    void OutputLikeSecondSellSignal(float* out, int count, const float* highs) const;
    
    /// @brief 输出新K线标记 (去包含后)
    /// @note 输出: 1=新K线, 0=被合并
    void OutputNewBar(float* out, int count) const;
    
    // ========================================================================
    // 辅助函数
    // ========================================================================
    
    /// @brief 清除所有计算结果
    void Clear();
    
    /// @brief 获取原始K线索引对应的合并K线索引
    int GetMergedIndex(int raw_index) const;
    
private:
    // 配置
    ChanConfig m_config;
    
    // 原始数据引用
    int m_raw_count;
    
    // 计算结果
    std::vector<KLine> m_merged_klines;     // 去包含后的K线
    std::vector<Fractal> m_fractals;        // 分型列表
    std::vector<Stroke> m_strokes;          // 笔列表
    std::vector<Pivot> m_pivots;            // 中枢列表
    std::vector<BiSequenceData> m_bi_sequence; // 递归引用序列
    
    // 均线数据（用于买卖点判断）
    std::vector<float> m_ma13;
    std::vector<float> m_ma26;
    
    // 原始索引到合并索引的映射
    std::vector<int> m_raw_to_merged;
    
    // 内部辅助函数
    bool HasIncludeRelation(const KLine& k1, const KLine& k2) const;
    void MergeKLine(KLine& target, const KLine& source, Direction dir);
    Direction DetermineDirection(const std::vector<KLine>& klines, int idx) const;
    
    bool IsFXValid(const Fractal& fx) const;
    bool CanFormStroke(const Fractal& fx1, const Fractal& fx2) const;
    
    // 阶段二辅助函数
    int CalculateDirection(int bar_idx) const;
    
    // 阶段三辅助函数
    bool CheckFiveDownPattern(int bar_idx) const;    // 五段下跌形态
    bool CheckFiveUpPattern(int bar_idx) const;      // 五段上涨形态
    bool CheckThreeDownPattern(int bar_idx) const;   // 三段下跌形态
    bool CheckThreeUpPattern(int bar_idx) const;     // 三段上涨形态
    bool CheckFirstSellKJA(int bar_idx) const;       // 一卖KJA条件
    bool CheckFirstSellKJB(int bar_idx) const;       // 一卖KJB条件
};

// ============================================================================
// 独立工具函数
// ============================================================================

/// @brief 判断两根K线是否存在包含关系
/// @param h1, l1 第一根K线的高低点
/// @param h2, l2 第二根K线的高低点
/// @return true=存在包含关系
inline bool HasInclude(float h1, float l1, float h2, float l2) {
    // K1包含K2 或 K2包含K1
    return (h1 >= h2 && l1 <= l2) || (h2 >= h1 && l2 <= l1);
}

/// @brief 计算笔的幅度
/// @param high 笔高点
/// @param low 笔低点
/// @return 幅度 = high - low
inline float CalcAmplitude(float high, float low) {
    return high - low;
}

/// @brief 计算百分比跌幅
/// @param prev_high 前高点
/// @param curr_low 当前低点
/// @return 跌幅百分比
inline float CalcDropPercent(float prev_high, float curr_low) {
    if (prev_high <= 0) return 0;
    return (prev_high - curr_low) / prev_high * 100.0f;
}

/// @brief 计算百分比涨幅
/// @param prev_low 前低点
/// @param curr_high 当前高点
/// @return 涨幅百分比
inline float CalcRisePercent(float prev_low, float curr_high) {
    if (prev_low <= 0) return 0;
    return (curr_high - prev_low) / prev_low * 100.0f;
}

} // namespace chan

#endif // CHAN_CORE_H
