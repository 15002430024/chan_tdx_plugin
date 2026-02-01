#pragma once
// ============================================================================
// 缠论通达信DLL插件 - 核心数据类型定义
// ============================================================================

#ifndef CHAN_TYPES_H
#define CHAN_TYPES_H

#include <vector>
#include <string>

namespace chan {

// ============================================================================
// 枚举类型
// ============================================================================

// 分型类型
enum class FractalType {
    NONE = 0,
    TOP = 1,      // 顶分型
    BOTTOM = -1   // 底分型
};

// 方向
enum class Direction {
    NONE = 0,
    UP = 1,       // 向上
    DOWN = -1     // 向下
};

// 信号类型
enum class SignalType {
    NONE = 0,
    BUY1 = 1,     // 一买
    BUY2 = 2,     // 二买
    BUY3 = 3,     // 三买
    SELL1 = -1,   // 一卖
    SELL2 = -2,   // 二卖
    SELL3 = -3    // 三卖
};

// 一买细分类型
enum class FirstBuyType {
    NONE = 0,
    TYPE_A = 1,     // 一买A型（有缺口，成功率89%）
    TYPE_B = 2,     // 一买B型（无缺口+幅度衰减，成功率85%）
    TYPE_AAA = 3    // 一买AAA型（增强型，成功率93%）
};

// 二买细分类型
enum class SecondBuyType {
    NONE = 0,
    TYPE_A = 1,     // 二买A型（三段下跌后）
    TYPE_B1 = 2,    // 二买B1型（五段+缺口）
    TYPE_B2 = 3     // 二买B2型（五段+无缺口）
};

// 三买细分类型
enum class ThirdBuyType {
    NONE = 0,
    TYPE_A = 1      // 三买（中枢突破后回踩不破）
};

// 一卖细分类型
enum class FirstSellType {
    NONE = 0,
    TYPE_A = 1,     // 一卖A型（有缺口）
    TYPE_B = 2,     // 一卖B型（无缺口+幅度衰减）
    TYPE_AAA = 3    // 一卖AAA型（增强型）
};

// 二卖细分类型
enum class SecondSellType {
    NONE = 0,
    TYPE_A = 1,     // 二卖A型（三段上涨后）
    TYPE_B1 = 2,    // 二卖B1型（五段+缺口）
    TYPE_B2 = 3     // 二卖B2型（五段+无缺口）
};

// 三卖细分类型
enum class ThirdSellType {
    NONE = 0,
    TYPE_A = 1      // 三卖（中枢跌破后反弹不回）
};

// ============================================================================
// 准买卖点类型 (阶段四)
// ============================================================================

// 准一买类型（条件放宽版）
enum class PreFirstBuyType {
    NONE = 0,
    TYPE_A = 1      // 准一买（时间窗口放宽至8，部分底部降低即可）
};

// 准二买类型（条件放宽版）
enum class PreSecondBuyType {
    NONE = 0,
    TYPE_A = 1      // 准二买（时间窗口放宽至10，中枢雏形即可）
};

// 准三买类型（条件放宽版）
enum class PreThirdBuyType {
    NONE = 0,
    TYPE_A = 1      // 准三买（回调接近但未触及中枢上沿）
};

// 类二买类型（强二买）
enum class LikeSecondBuyType {
    NONE = 0,
    TYPE_A = 1,     // 类二买A（成功率85%）
    TYPE_AAA = 2    // 类二买AAA（最强买，成功率90%）
};

// 准一卖类型（镜像对称）
enum class PreFirstSellType {
    NONE = 0,
    TYPE_A = 1
};

// 准二卖类型（镜像对称）
enum class PreSecondSellType {
    NONE = 0,
    TYPE_A = 1
};

// 准三卖类型（镜像对称）
enum class PreThirdSellType {
    NONE = 0,
    TYPE_A = 1
};

// 类二卖类型（镜像对称）
enum class LikeSecondSellType {
    NONE = 0,
    TYPE_A = 1,
    TYPE_AAA = 2
};

// ============================================================================
// K线结构
// ============================================================================

struct KLine {
    int      index;         // 原始K线索引
    float    high;          // 最高价
    float    low;           // 最低价
    float    open;          // 开盘价
    float    close;         // 收盘价
    float    volume;        // 成交量
    float    amount;        // 成交额
    bool     is_merged;     // 是否为合并K线
    int      merge_start;   // 合并起始索引
    int      merge_end;     // 合并结束索引
    
    KLine() : index(0), high(0), low(0), open(0), close(0), 
              volume(0), amount(0), is_merged(false), 
              merge_start(0), merge_end(0) {}
};

// ============================================================================
// 分型结构
// ============================================================================

struct Fractal {
    int          index;       // 分型中间K线的索引（合并后）
    FractalType  type;        // 分型类型
    float        price;       // 极值价格 (顶分型=高点, 底分型=低点)
    int          kline_idx;   // 对应原始K线索引
    bool         is_valid;    // 是否有效
    int          strength;    // 强度 1-3
    
    Fractal() : index(0), type(FractalType::NONE), price(0), 
                kline_idx(0), is_valid(true), strength(1) {}
};

// ============================================================================
// 笔结构
// ============================================================================

struct Stroke {
    int        id;            // 笔的唯一ID
    int        start_idx;     // 起点K线索引
    int        end_idx;       // 终点K线索引
    Direction  direction;     // 方向
    float      high;          // 笔的最高点
    float      low;           // 笔的最低点
    float      power;         // 力度 (用于背驰比较)
    int        kline_count;   // 包含的K线数量
    
    Fractal    start_fx;      // 起点分型
    Fractal    end_fx;        // 终点分型
    
    Stroke() : id(0), start_idx(0), end_idx(0), direction(Direction::NONE),
               high(0), low(0), power(0), kline_count(0) {}
};

// ============================================================================
// 线段结构
// ============================================================================

struct Segment {
    int              id;            // 线段ID
    int              start_idx;     // 起点索引
    int              end_idx;       // 终点索引
    Direction        direction;     // 方向
    float            high;          // 最高点
    float            low;           // 最低点
    std::vector<int> stroke_ids;    // 包含的笔ID列表
    int              stroke_count;  // 笔的数量 (>=3)
    
    Segment() : id(0), start_idx(0), end_idx(0), direction(Direction::NONE),
                high(0), low(0), stroke_count(0) {}
};

// ============================================================================
// 中枢结构
// ============================================================================

struct Pivot {
    int        id;                // 中枢ID
    int        start_stroke_id;   // 起始笔ID
    int        end_stroke_id;     // 结束笔ID
    int        start_idx;         // 起点K线索引
    int        end_idx;           // 终点K线索引
    
    float      ZG;                // 中枢高点
    float      ZD;                // 中枢低点
    float      ZZ;                // 中枢中轴 = (ZG + ZD) / 2
    float      GG;                // 中枢最高点 (波动高点)
    float      DD;                // 中枢最低点 (波动低点)
    
    int        level;             // 级别 (0=本级, 1=升级...)
    int        stroke_count;      // 包含笔数
    Direction  direction;         // 中枢方向 (由进入段决定)
    
    bool       is_extended;       // 是否扩展
    bool       is_upgraded;       // 是否升级
    
    Pivot() : id(0), start_stroke_id(0), end_stroke_id(0),
              start_idx(0), end_idx(0), ZG(0), ZD(0), ZZ(0), GG(0), DD(0),
              level(0), stroke_count(0), direction(Direction::NONE),
              is_extended(false), is_upgraded(false) {}
};

// ============================================================================
// 买卖点结构
// ============================================================================

struct Signal {
    int         index;            // K线索引
    SignalType  type;             // 信号类型
    float       price;            // 触发价格
    int         strength;         // 强度 1-3
    int         pivot_id;         // 关联中枢ID
    bool        has_divergence;   // 是否背驰
    std::string reason;           // 信号原因描述
    
    Signal() : index(0), type(SignalType::NONE), price(0),
               strength(0), pivot_id(0), has_divergence(false) {}
};

// ============================================================================
// 递归引用结构 - 用于存储GG/DD序列
// ============================================================================

/// @brief 递归引用数据 - 存储每根K线对应的GG/DD序列
struct BiSequenceData {
    // 顶点价格序列（GG1=最近顶点，GG2=次近顶点...）
    float GG[6];   // GG1-GG5，索引0不用
    
    // 底点价格序列（DD1=最近底点，DD2=次近底点...）
    float DD[6];   // DD1-DD5，索引0不用
    
    // 顶点距离当前K线的K线数
    int HH[6];     // HH1-HH5
    
    // 底点距离当前K线的K线数
    int LL[6];     // LL1-LL5
    
    // 方向判断
    int direction; // 1=下跌后（底分型），-1=上涨后（顶分型），0=震荡
    
    // 当前K线索引
    int kline_idx;
    
    BiSequenceData() : direction(0), kline_idx(0) {
        for (int i = 0; i < 6; ++i) {
            GG[i] = 0;
            DD[i] = 0;
            HH[i] = 0;
            LL[i] = 0;
        }
    }
};

// ============================================================================
// 幅度计算辅助结构
// ============================================================================

/// @brief 幅度条件检查结果
struct AmplitudeCheck {
    bool kja_valid;      // 一买KJA条件是否满足（有缺口+幅度确认）
    bool kjb_valid;      // 一买KJB条件是否满足（无缺口+幅度衰减）
    bool l2b_valid;      // 类二买幅度条件是否满足
    bool has_gap;        // 是否存在缺口 (GG1 < DD3)
    bool five_down;      // 是否满足五段下跌形态
    
    float amp1;          // 第1段幅度 (GG1-DD1)
    float amp2;          // 第2段幅度 (GG2-DD2)
    float amp3;          // 第3段幅度 (GG3-DD3)
    float amp_total;     // 整体幅度
    
    AmplitudeCheck() : kja_valid(false), kjb_valid(false), l2b_valid(false),
                       has_gap(false), five_down(false),
                       amp1(0), amp2(0), amp3(0), amp_total(0) {}
};

} // namespace chan

#endif // CHAN_TYPES_H
