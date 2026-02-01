# API 文档

## 一、DLL概述

| 属性 | 值 |
|------|-----|
| 文件名 | chan.dll |
| 架构 | Win32 (x86) |
| 调用约定 | __stdcall |
| 编码 | UTF-8 |
| 导出函数数 | 24 |

---

## 二、函数签名

所有导出函数遵循通达信DLL接口规范：

```cpp
extern "C" __declspec(dllexport) void __stdcall FunctionName(
    float* pHigh,    // 最高价数组
    float* pLow,     // 最低价数组
    float* pOpen,    // 开盘价数组
    float* pClose,   // 收盘价数组
    float* pVol,     // 成交量数组
    float* pAmount,  // 成交额数组
    int nCount,      // 数据长度
    float* pOut,     // 输出数组
    float* pParam    // 附加参数（可选）
);
```

---

## 三、导出函数列表

### 3.1 基础函数 (8个)

#### CHAN_FX - 分型识别
```cpp
void CHAN_FX_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| 1 | 顶分型 |
| -1 | 底分型 |
| 0 | 无分型 |

---

#### CHAN_BI - 笔端点
```cpp
void CHAN_BI_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| >0 | 笔端点价格（高/低点） |
| 0 | 非端点 |

---

#### CHAN_ZS_H - 中枢高点
```cpp
void CHAN_ZS_H_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| >0 | 中枢上沿ZG |
| 0 | 不在中枢内 |

---

#### CHAN_ZS_L - 中枢低点
```cpp
void CHAN_ZS_L_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| >0 | 中枢下沿ZD |
| 0 | 不在中枢内 |

---

#### CHAN_ZS_Z - 中枢中轴
```cpp
void CHAN_ZS_Z_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| >0 | 中枢中轴 (ZG+ZD)/2 |
| 0 | 不在中枢内 |

---

#### CHAN_DIR - 方向判断
```cpp
void CHAN_DIR_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| 1 | 下跌趋势后（适合判断买点） |
| -1 | 上涨趋势后（适合判断卖点） |
| 0 | 震荡 |

---

#### CHAN_NEWBAR - 新K线标记
```cpp
void CHAN_NEWBAR_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| 1 | 去包含后保留的K线 |
| 0 | 被合并的K线 |

---

### 3.2 递归引用函数 (10个)

#### CHAN_GG - 顶点价格
```cpp
void CHAN_GG_Calc(..., float* pParam)
```
- **参数**: pParam[0] = n (1-5)
- **返回**: GGn = 第n近的顶点价格

---

#### CHAN_DD - 底点价格
```cpp
void CHAN_DD_Calc(..., float* pParam)
```
- **参数**: pParam[0] = n (1-5)
- **返回**: DDn = 第n近的底点价格

---

#### CHAN_HH - 顶点距离
```cpp
void CHAN_HH_Calc(..., float* pParam)
```
- **参数**: pParam[0] = n (1-5)
- **返回**: HHn = 第n近顶点距当前K线数

---

#### CHAN_LL - 底点距离
```cpp
void CHAN_LL_Calc(..., float* pParam)
```
- **参数**: pParam[0] = n (1-5)
- **返回**: LLn = 第n近底点距当前K线数

---

### 3.3 标准买卖点函数 (6个)

#### CHAN_BUY - 标准买点
```cpp
void CHAN_BUY_Calc(...)
```
| 返回值 | 类型 | 说明 |
|--------|------|------|
| 1 | 一买A | 有缺口型 |
| 2 | 一买B | 无缺口+幅度衰减 |
| 3 | 一买AAA | 增强型（成功率93%） |
| 11 | 二买A | 三段下跌后 |
| 12 | 二买B1 | 五段+缺口 |
| 13 | 二买B2 | 五段+无缺口 |
| 21 | 三买 | 中枢突破后回踩 |
| 0 | 无信号 | |

---

#### CHAN_SELL - 标准卖点
```cpp
void CHAN_SELL_Calc(...)
```
| 返回值 | 类型 | 说明 |
|--------|------|------|
| -1 | 一卖A | 有缺口型 |
| -2 | 一卖B | 无缺口+幅度衰减 |
| -3 | 一卖AAA | 增强型 |
| -11 | 二卖A | 三段上涨后 |
| -12 | 二卖B1 | 五段+缺口 |
| -13 | 二卖B2 | 五段+无缺口 |
| -21 | 三卖 | 中枢跌破后反弹 |
| 0 | 无信号 | |

---

### 3.4 综合信号函数 (2个)

#### CHAN_BUYX - 综合买点
```cpp
void CHAN_BUYX_Calc(...)
```
| 返回值 | 类型 | 说明 |
|--------|------|------|
| 1 | 一买 | 标准一买 |
| 2 | 二买 | 标准二买 |
| 3 | 三买 | 标准三买 |
| 11 | 准一买 | 条件放宽 |
| 12 | 准二买 | 条件放宽 |
| 13 | 准三买 | 条件放宽 |
| 21 | 类二买A | 成功率85% |
| 22 | 类二买AAA | 成功率90% |
| 0 | 无信号 | |

**优先级**: 标准买点 > 类买点 > 准买点

---

#### CHAN_SELLX - 综合卖点
```cpp
void CHAN_SELLX_Calc(...)
```
| 返回值 | 类型 | 说明 |
|--------|------|------|
| -1 | 一卖 | 标准一卖 |
| -2 | 二卖 | 标准二卖 |
| -3 | 三卖 | 标准三卖 |
| -11 | 准一卖 | 条件放宽 |
| -12 | 准二卖 | 条件放宽 |
| -13 | 准三卖 | 条件放宽 |
| -21 | 类二卖A | |
| -22 | 类二卖AAA | |
| 0 | 无信号 | |

---

### 3.5 独立信号函数 (4个)

#### CHAN_PREBUY - 准买点
```cpp
void CHAN_PREBUY_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| 11 | 准一买 |
| 12 | 准二买 |
| 13 | 准三买 |
| 0 | 无信号 |

---

#### CHAN_PRESELL - 准卖点
```cpp
void CHAN_PRESELL_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| -11 | 准一卖 |
| -12 | 准二卖 |
| -13 | 准三卖 |
| 0 | 无信号 |

---

#### CHAN_LIKE2B - 类二买
```cpp
void CHAN_LIKE2B_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| 21 | 类二买A |
| 22 | 类二买AAA |
| 0 | 无信号 |

---

#### CHAN_LIKE2S - 类二卖
```cpp
void CHAN_LIKE2S_Calc(...)
```
| 返回值 | 说明 |
|--------|------|
| -21 | 类二卖A |
| -22 | 类二卖AAA |
| 0 | 无信号 |

---

## 四、核心类 API

### 4.1 ChanCore 类

```cpp
namespace chan {

class ChanCore {
public:
    // 构造函数
    ChanCore();
    explicit ChanCore(const ChanConfig& config);
    
    // 配置
    void SetConfig(const ChanConfig& config);
    const ChanConfig& GetConfig() const;
    
    // 主处理流程
    int Analyze(const float* highs, const float* lows,
                const float* closes, const float* volumes, int count);
    
    // 阶段一：基础算法
    int RemoveInclude(const float* highs, const float* lows, int count);
    int CheckFX();   // 返回分型数量
    int CheckBI();   // 返回笔数量
    int CheckZS();   // 返回中枢数量
    
    // 阶段二：递归引用
    void BuildBiSequence(int current_bar_idx);
    float GetGG(int kline_idx, int n) const;  // n=1~5
    float GetDD(int kline_idx, int n) const;
    int GetHH(int kline_idx, int n) const;
    int GetLL(int kline_idx, int n) const;
    int GetDirection(int kline_idx) const;
    
    // 阶段三：买卖点判断
    FirstBuyType CheckFirstBuy(int bar_idx, float low) const;
    SecondBuyType CheckSecondBuy(int bar_idx, float low) const;
    ThirdBuyType CheckThirdBuy(int bar_idx, float low) const;
    FirstSellType CheckFirstSell(int bar_idx, float high) const;
    SecondSellType CheckSecondSell(int bar_idx, float high) const;
    ThirdSellType CheckThirdSell(int bar_idx, float high) const;
    
    // 阶段四：准/类买卖点
    PreFirstBuyType CheckPreFirstBuy(int bar_idx, float low) const;
    PreSecondBuyType CheckPreSecondBuy(int bar_idx, float low) const;
    PreThirdBuyType CheckPreThirdBuy(int bar_idx, float low) const;
    LikeSecondBuyType CheckLikeSecondBuy(int bar_idx, float low) const;
    
    // 输出函数
    void OutputFX(float* out, int count) const;
    void OutputBI(float* out, int count) const;
    void OutputZS_H(float* out, int count) const;
    void OutputZS_L(float* out, int count) const;
    void OutputZS_Z(float* out, int count) const;
    void OutputDirection(float* out, int count) const;
    void OutputCombinedBuySignal(float* out, int count, const float* lows) const;
    void OutputCombinedSellSignal(float* out, int count, const float* highs) const;
    
    // 获取计算结果
    const std::vector<KLine>& GetMergedKLines() const;
    const std::vector<Fractal>& GetFractals() const;
    const std::vector<Stroke>& GetStrokes() const;
    const std::vector<Pivot>& GetPivots() const;
};

} // namespace chan
```

---

### 4.2 ChanConfig 结构

```cpp
struct ChanConfig {
    int min_bi_len = 5;           // 笔最小K线数量
    int min_fx_distance = 1;      // 分型最小间隔K线数
    int min_zs_bi_count = 3;      // 中枢最小笔数量
    bool strict_bi = true;        // 严格笔定义
    bool enable_like_signals = true;  // 启用类买卖点
    bool enable_pre_signals = true;   // 启用准买卖点
};
```

---

### 4.3 枚举类型

```cpp
enum class FirstBuyType {
    NONE = 0,
    TYPE_A = 1,     // 一买A型（有缺口，成功率89%）
    TYPE_B = 2,     // 一买B型（无缺口+幅度衰减，成功率85%）
    TYPE_AAA = 3    // 一买AAA型（增强型，成功率93%）
};

enum class SecondBuyType {
    NONE = 0,
    TYPE_A = 1,     // 二买A型（三段下跌后）
    TYPE_B1 = 2,    // 二买B1型（五段+缺口）
    TYPE_B2 = 3     // 二买B2型（五段+无缺口）
};

enum class ThirdBuyType {
    NONE = 0,
    TYPE_A = 1      // 三买（中枢突破后回踩不破）
};

enum class LikeSecondBuyType {
    NONE = 0,
    TYPE_A = 1,     // 类二买A（成功率85%）
    TYPE_AAA = 2    // 类二买AAA（成功率90%）
};
```

---

## 五、错误处理

所有函数均不抛出异常，输入无效时返回0或保持输出数组不变。

---

## 六、线程安全

- ChanCore实例不是线程安全的
- 每个线程应使用独立的ChanCore实例
- DLL的全局状态是线程安全的

---

## 七、性能指标

| 指标 | 数值 |
|------|------|
| 100K K线全量分析 | < 10ms |
| 增量更新 | < 1ms |
| 内存占用 | < 50MB (100K K线) |
