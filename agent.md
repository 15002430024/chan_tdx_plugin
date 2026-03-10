# 缠论通达信DLL项目开发进度追踪

> 本文档由 AI Agent 自动维护，记录项目开发进度和实现细节

## 📋 项目概述

- **项目名称**: 通达信缠论DLL插件 (chan_tdx_plugin)
- **创建日期**: 2026-01-12
- **最后更新**: 2026-03-10
- **当前状态**: 🚧 开发中 - v7.5.1 Phase 3 完成（版本号统一），下一步 Phase 4 回归测试发布闸门

---

## ✅ 已实现功能

### 模块 A: 项目框架

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| CMake 构建系统 | ✅ 完成 | 2026-03-10 | [v7.5] chan DLL 链接 tdx_standard.cpp + chan_core.cpp + logger.cpp |
| 通达信接口框架 | ✅ 完成 | 2026-01-12 | 18个导出函数已实现 |
| 日志系统 | ✅ 完成 | 2026-01-12 | 支持文件和控制台输出 |
| 基础数据类型定义 | ✅ 完成 | 2026-01-12 | KLine/Fractal/Stroke/Pivot/BiSequenceData |
| 单元测试框架 | ✅ 完成 | 2026-03-08 | 50个测试用例全部通过（含v7.4新增10个） |

### 模块 B: 核心算法（阶段一）

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| K线去包含处理 | ✅ 完成 | 2026-02-01 | RemoveInclude() 支持向上/向下趋势 |
| 分型识别算法 | ✅ 完成 | 2026-02-01 | CheckFX() 识别顶底分型，处理连续同类型 |
| 笔识别算法 | ✅ 完成 | 2026-03-08 | CheckBI() 状态机重写：候选终点+极值追踪+未完成笔输出 |
| 中枢识别 | ✅ 完成 | 2026-03-08 | CheckZS() 前三笔锁定ZG/ZD+延伸不压缩+排除未完成笔 |

### 模块 B2: 递归引用系统（阶段二）

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| GG序列 | ✅ 完成 | 2026-02-01 | GG1-GG5 顶点价格序列 |
| DD序列 | ✅ 完成 | 2026-02-01 | DD1-DD5 底点价格序列 |
| HH序列 | ✅ 完成 | 2026-02-01 | HH1-HH5 顶点距离 |
| LL序列 | ✅ 完成 | 2026-02-01 | LL1-LL5 底点距离 |
| 方向判断 | ✅ 完成 | 2026-02-01 | GetDirection() 1=看涨, -1=看跌, 0=震荡 |
| KJA幅度条件 | ✅ 完成 | 2026-02-01 | 一买有缺口型幅度检查 |
| KJB幅度条件 | ✅ 完成 | 2026-02-01 | 一买无缺口型幅度检查 |
| 二买幅度条件 | ✅ 完成 | 2026-02-01 | DD1 > DD2 检查 |

### 模块 C: 买卖点识别（阶段三）

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| 一买判断 | ✅ 完成 | 2026-02-01 | A/B/AAA三种类型，支持缺口和幅度条件 |
| 二买判断 | ✅ 完成 | 2026-02-01 | A/B1/B2三种类型，基于波浪数和缺口 |
| 三买判断 | ✅ 完成 | 2026-02-01 | 中枢突破后回踩不进中枢 |
| 一卖判断 | ✅ 完成 | 2026-02-01 | 镜像对称于一买 |
| 二卖判断 | ✅ 完成 | 2026-02-01 | 镜像对称于二买 |
| 三卖判断 | ✅ 完成 | 2026-02-01 | 中枢跌破后反弹不进中枢 |

### 模块 D: 准买卖点与综合信号（阶段四）

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| 准一买判断 | ✅ 完成 | 2026-02-01 | 时间窗口放宽至8，部分底部降低即可 |
| 准二买判断 | ✅ 完成 | 2026-02-01 | 时间窗口放宽至10，中枢雏形即可 |
| 准三买判断 | ✅ 完成 | 2026-02-01 | 回调接近但未触及中枢上沿 |
| 类二买判断 | ✅ 完成 | 2026-02-01 | A型(85%)、AAA型(90%成功率) |
| 综合买点信号 | ✅ 完成 | 2026-02-01 | 优先级：标准>类>准，可配置启用/禁用 |
| 综合卖点信号 | ✅ 完成 | 2026-02-01 | 镜像对称于买点 |

### 模块 E: DLL封装与优化（阶段五）

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| DLL接口扩展至24个 | ✅ 完成 | 2026-02-01 | 新增6个导出函数 |
| INI配置文件系统 | ✅ 完成 | 2026-02-01 | CZSC.ini配置读取，支持热加载 |
| 中枢中轴输出 | ✅ 完成 | 2026-02-01 | OutputZS_Z() 输出 (ZG+ZD)/2 |
| 准买点独立输出 | ✅ 完成 | 2026-02-01 | OutputPreBuySignal() 输出11/12/13 |
| 准卖点独立输出 | ✅ 完成 | 2026-02-01 | OutputPreSellSignal() 输出-11/-12/-13 |
| 类二买独立输出 | ✅ 完成 | 2026-02-01 | OutputLikeSecondBuySignal() 输出21/22 |
| 类二卖独立输出 | ✅ 完成 | 2026-02-01 | OutputLikeSecondSellSignal() 输出-21/-22 |
| 新K线标记输出 | ✅ 完成 | 2026-02-01 | OutputNewBar() 输出1=保留/0=合并 |
| 性能优化 | ✅ 完成 | 2026-02-01 | 100K K线仅需4ms，远超目标 |

### 模块 F: 测试与发布（阶段六）

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| 集成测试用例 | ✅ 完成 | 2026-02-01 | 8个集成测试覆盖主要模块 |
| 文档完善 | ✅ 完成 | 2026-02-01 | README/INSTALL/USAGE/API/CHANGELOG |
| 通达信公式示例 | ✅ 完成 | 2026-02-01 | 5个示例公式（分型/笔/中枢/买卖点） |
| MIT开源协议 | ✅ 完成 | 2026-02-01 | LICENSE文件 |

### 模块 G: 标准接口DLL (v6.0) - 速查手册完整实现

| 功能 | 状态 | 实现日期 | 说明 |
|------|------|----------|------|
| 标准接口封装 | ✅ 完成 | 2026-02-01 | tdx_standard.cpp v6.0，完全实现速查手册 |
| 分型识别 (编号1) | ✅ 完成 | 2026-02-01 | FenXing() 返回1/-1/0 |
| 笔端点 (编号2) | ✅ 完成 | 2026-02-01 | BiDuanDian() 返回1=顶/-1=底/0=非端点 |
| 中枢高点 (编号3) | ✅ 完成 | 2026-02-01 | ZhongShuGao() 返回ZG |
| 中枢低点 (编号4) | ✅ 完成 | 2026-02-01 | ZhongShuDi() 返回ZD |
| 中枢中轴 (编号5) | ✅ 完成 | 2026-02-01 | ZhongShuZhong() 返回(ZG+ZD)/2 |
| 笔方向 (编号6) | ✅ 完成 | 2026-02-01 | BiDirection() 返回1/-1/0 |
| 买点信号 (编号7) | ✅ 完成 | 2026-02-01 | BuySignal() 完整123买+准买点 |
| 卖点信号 (编号8) | ✅ 完成 | 2026-02-01 | SellSignal() 完整123卖+准卖点 |
| 新K线标记 (编号9) | ✅ 完成 | 2026-02-01 | NewBar() 返回1/0 |
| 测试函数 (编号10) | ✅ 完成 | 2026-02-01 | TestFunc() 返回K线序号 |
| 方向判断 (编号11) | ✅ 完成 | 2026-02-01 | Direction() 返回1/-1/0 |
| GG1 (编号12) | ✅ 完成 | 2026-02-01 | OutputGG1() 最近顶点价格 |
| DD1 (编号13) | ✅ 完成 | 2026-02-01 | OutputDD1() 最近底点价格 |
| LL1 (编号14) | ✅ 完成 | 2026-02-01 | OutputLL1() 最近底距离 |
| HH1 (编号15) | ✅ 完成 | 2026-02-01 | OutputHH1() 最近顶距离 |
| MA13 (编号16) | ✅ 完成 | 2026-02-01 | OutputMA13() 13周期均线 |
| MA26 (编号17) | ✅ 完成 | 2026-02-01 | OutputMA26() 26周期均线 |
| 中枢开始 (编号18) | ✅ 完成 | 2026-03-06 | ZhongShuKaiShi() 1=下跌中枢开始, 2=上涨中枢开始 |
| 中枢结束 (编号19) | ✅ 完成 | 2026-03-06 | ZhongShuJieShu() 1=下跌中枢结束, 2=上涨中枢结束 |
| 笔高点 (编号20) | ✅ 完成 | 2026-03-06 | BiGaoDian() 顶端点价格, 非顶为0 |
| 笔低点 (编号21) | ✅ 完成 | 2026-03-06 | BiDiDian() 底端点价格, 非底为0 |
| 中枢方向 (编号22) | ✅ 完成 | 2026-03-08 | ZhongShuFangXiang() 1=上涨中枢, -1=下跌中枢, 0=不在中枢内 |

---

## 🔌 接口定义

### 通达信标准接口 (v5.0 推荐使用)

```cpp
// 函数签名（固定5参数）
typedef void(*pPluginFUNC)(int DataLen, float* pfOUT, 
                           float* pfINa, float* pfINb, float* pfINc);

// 注册结构（1字节对齐）
#pragma pack(push, 1)
typedef struct {
    unsigned short nFuncMark;   // 函数编号
    pPluginFUNC pCallFunc;      // 函数指针
} PluginTCalcFuncInfo;
#pragma pack(pop)

// 导出函数
extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun);
```

### 函数编号对照表

| 编号 | 函数名 | 公式调用 | 返回值 |
|------|--------|----------|--------|
| 1 | FenXing | `TDXDLL1(1, H, L, C)` | 1=顶分型, -1=底分型, 0=无 |
| 2 | BiDuanDian | `TDXDLL1(2, H, L, C)` | 1=顶端点, -1=底端点, 0=非端点 |
| 3 | ZhongShuGao | `TDXDLL1(3, H, L, C)` | 中枢高点ZG |
| 4 | ZhongShuDi | `TDXDLL1(4, H, L, C)` | 中枢低点ZD |
| 5 | ZhongShuZhong | `TDXDLL1(5, H, L, C)` | 中枢中轴 |
| 6 | BiDirection | `TDXDLL1(6, H, L, C)` | 1=向上笔, -1=向下笔 |
| 7 | BuySignal | `TDXDLL1(7, H, L, C)` | 1/2=一买A/B, 11/12/13=二买, 21=三买, 31/32/33=准买点 |
| 8 | SellSignal | `TDXDLL1(8, H, L, C)` | -1/-2/-3=一卖A/B/C, -11/-12/-13=二卖, -21=三卖, -31~-33=准卖点 |
| 9 | NewBar | `TDXDLL1(9, H, L, C)` | 1=保留, 0=被合并 |
| 10 | TestFunc | `TDXDLL1(10, H, L, C)` | K线序号 |
| 11 | Direction | `TDXDLL1(11, H, L, C)` | 1=下跌后(买), -1=上涨后(卖) |
| 12 | OutputGG1 | `TDXDLL1(12, H, L, C)` | 最近顶点价格GG1 |
| 13 | OutputDD1 | `TDXDLL1(13, H, L, C)` | 最近底点价格DD1 |
| 14 | OutputLL1 | `TDXDLL1(14, H, L, C)` | 最近底距离LL1 |
| 15 | OutputHH1 | `TDXDLL1(15, H, L, C)` | 最近顶距离HH1 |
| 16 | OutputMA13 | `TDXDLL1(16, H, L, C)` | 13周期均线 |
| 17 | OutputMA26 | `TDXDLL1(17, H, L, C)` | 26周期均线 |
| 18 | ZhongShuKaiShi | `TDXDLL1(18, H, L, C)` | 1=下跌中枢开始, 2=上涨中枢开始 |
| 19 | ZhongShuJieShu | `TDXDLL1(19, H, L, C)` | 1=下跌中枢结束, 2=上涨中枢结束 |
| 20 | BiGaoDian | `TDXDLL1(20, H, L, C)` | 顶端点价格, 非顶为0 |
| 21 | BiDiDian | `TDXDLL1(21, H, L, C)` | 底端点价格, 非底为0 |
| 22 | ZhongShuFangXiang | `TDXDLL1(22, H, L, C)` | 中枢方向持续输出: 1=上涨, -1=下跌, 0=无 |

### 旧版接口 (保留兼容)

```cpp
// 阶段一：基础算法 (8个)
void CHAN_FX_Calc(...)   // 分型标记: 1=顶, -1=底, 0=无
void CHAN_BI_Calc(...)   // 笔端点价格
void CHAN_DUAN_Calc(...) // 线段端点 (待实现)
void CHAN_ZS_H_Calc(...) // 中枢高点ZG
void CHAN_ZS_L_Calc(...) // 中枢低点ZD
void CHAN_BUY_Calc(...)  // 标准买点: 1-3=一买A/B/AAA, 11-13=二买A/B1/B2, 21=三买A
void CHAN_SELL_Calc(...) // 标准卖点: -1~-3=一卖, -11~-13=二卖, -21=三卖
void CHAN_BC_Calc(...)   // 背驰 (待实现)

// 阶段二：递归引用系统 (6个)
void CHAN_DIR_Calc(...)  // 方向判断
void CHAN_GG_Calc(...)   // 顶点价格GGn
void CHAN_DD_Calc(...)   // 底点价格DDn
void CHAN_HH_Calc(...)   // 顶点距离HHn
void CHAN_LL_Calc(...)   // 底点距离LLn
void CHAN_AMP_Calc(...)  // 幅度条件检查

// 阶段四：综合信号 (2个)
void CHAN_BUYX_Calc(...) // 综合买点: 1=一买, 2=二买, 3=三买, 11=准一买, 12=准二买, 13=准三买, 21=类二买
void CHAN_SELLX_Calc(...) // 综合卖点: -1=一卖, -2=二卖, -3=三卖, -11=准一卖, -12=准二卖, -13=准三卖, -21=类二卖

// 阶段五：扩展信号 (6个)
void CHAN_ZS_Z_Calc(...)   // 中枢中轴 (ZG+ZD)/2
void CHAN_PREBUY_Calc(...) // 准买点: 11=准一买, 12=准二买, 13=准三买
void CHAN_PRESELL_Calc(...) // 准卖点: -11=准一卖, -12=准二卖, -13=准三卖
void CHAN_LIKE2B_Calc(...) // 类二买: 21=A型, 22=AAA型
void CHAN_LIKE2S_Calc(...) // 类二卖: -21=A型, -22=AAA型
void CHAN_NEWBAR_Calc(...) // 新K线标记: 1=保留, 0=被合并
```

### 核心算法类 (ChanCore)

```cpp
namespace chan {
    class ChanCore {
        // 主处理流程
        int Analyze(const float* highs, const float* lows, 
                    const float* closes, const float* volumes, int count);
        
        // 阶段一：基础算法
        int RemoveInclude(const float* highs, const float* lows, int count);
        int CheckFX();   // 返回分型数量
        int CheckBI();   // 返回笔数量
        int CheckZS();   // 返回中枢数量
        
        // 阶段二：递归引用系统
        void BuildBiSequence(int current_bar_idx);
        float GetGG(int kline_idx, int n) const;  // GG1-GG5
        float GetDD(int kline_idx, int n) const;  // DD1-DD5
        int GetHH(int kline_idx, int n) const;    // HH1-HH5
        int GetLL(int kline_idx, int n) const;    // LL1-LL5
        int GetDirection(int kline_idx) const;    // 方向判断
        
        // 阶段二：幅度检查
        bool CheckFirstBuyKJA(int bar_idx) const;
        bool CheckFirstBuyKJB(int bar_idx) const;
        bool CheckSecondBuyAmplitude(int bar_idx) const;
        AmplitudeCheck GetAmplitudeCheck(int bar_idx) const;
        
        // 输出函数
        void OutputFX(float* out, int count) const;
        void OutputBI(float* out, int count) const;
        void OutputZS_H(float* out, int count) const;
        void OutputZS_L(float* out, int count) const;
        void OutputDirection(float* out, int count) const;
        void OutputGG(float* out, int count, int n) const;
        void OutputDD(float* out, int count, int n) const;
        void OutputHH(float* out, int count, int n) const;
        void OutputLL(float* out, int count, int n) const;
    };
    
    struct ChanConfig {
        int min_bi_len = 5;       // 笔最小K线数
        int min_fx_distance = 1;  // 分型最小间隔
        int min_zs_bi_count = 3;  // 中枢最小笔数
        bool strict_bi = true;    // 严格笔定义
    };
    
    struct BiSequenceData {
        float GG[6];   // GG[1]-GG[5] 顶点价格
        float DD[6];   // DD[1]-DD[5] 底点价格
        int HH[6];     // HH[1]-HH[5] 顶点距离
        int LL[6];     // LL[1]-LL[5] 底点距离
        int direction; // 方向判断
    };
    
    struct AmplitudeCheck {
        bool kja_valid;   // KJA条件
        bool kjb_valid;   // KJB条件
        bool l2b_valid;   // 二买幅度
        bool has_gap;     // 缺口条件
        bool five_down;   // 五段下跌
        float amp1, amp2, amp3; // 各段幅度
    };
}
```

### 工具函数

```cpp
bool HasInclude(float h1, float l1, float h2, float l2);
float CalcAmplitude(float high, float low);
float CalcDropPercent(float prev_high, float curr_low);
float CalcRisePercent(float prev_low, float curr_high);
```

---

## 🔗 依赖关系

### 模块依赖图

```
tdx_interface.cpp (通达信入口)
  ├── chan_core.h/cpp (核心算法)
  │     ├── RemoveInclude() 去包含
  │     ├── CheckFX() 分型识别
  │     ├── CheckBI() 笔识别
  │     └── CheckZS() 中枢识别
  ├── chan_types.h (数据类型)
  └── logger.h/cpp (日志)
```

### 文件结构

```
chan_tdx_plugin/
├── include/
│   ├── chan_core.h      # 核心算法声明
│   ├── chan_types.h     # 数据类型定义
│   ├── tdx_interface.h  # 通达信接口
│   ├── config_reader.h  # INI配置读取器
│   └── logger.h         # 日志系统
├── src/
│   ├── chan_core.cpp    # 核心算法实现
│   ├── tdx_interface.cpp# 通达信接口实现
│   ├── config_reader.cpp# INI配置读取实现
│   ├── dllmain.cpp      # DLL入口
│   └── logger.cpp       # 日志实现
├── test/
│   └── test_chan_core.cpp # 32个单元测试
├── build/bin/Release/
│   ├── chan.dll         # 编译输出
│   └── test_chan_core.exe
├── CZSC.ini             # 配置文件模板
└── CMakeLists.txt
```

### 外部依赖

| 包名 | 版本 | 用途 |
|------|------|------|
| MSVC | 2019+ | C++17 编译 |
| CMake | 3.15+ | 构建系统 |

---

## ⚠️ 注意事项

### 重要约定

1. **32位编译**: 通达信只支持32位DLL，必须用 `-A Win32` 编译
2. **调用约定**: 所有导出函数必须使用 `__stdcall`
3. **价格精度**: 通达信价格为float类型，无需放大处理
4. **内存管理**: 输出数组由通达信分配，DLL只需填充数据

### 已知问题

1. **初始方向判断**: 去包含处理前两根K线时方向可能为0，已通过向后查看处理
2. **分型连续性**: 连续同类型分型需取极值，已在CheckFX中实现
3. **⚠️ 通达信接口规范**: 不支持自定义函数名，必须用 `TDXDLL1(编号, H, L, C)` 调用

### 通达信DLL接口规范（重要！）

```cpp
// 1. 函数签名（固定5参数）
typedef void(*pPluginFUNC)(int DataLen, float* pfOUT, 
                           float* pfINa, float* pfINb, float* pfINc);

// 2. 注册结构（必须1字节对齐，仅6字节）
#pragma pack(push, 1)
typedef struct {
    unsigned short nFuncMark;   // 函数编号
    pPluginFUNC pCallFunc;      // 函数指针
} PluginTCalcFuncInfo;
#pragma pack(pop)

// 3. 注册函数（返回BOOL，参数为二级指针）
extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun);

// 4. 注册数组必须以 {0, NULL} 结尾
PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1, (pPluginFUNC)&FenXing},
    {2, (pPluginFUNC)&BiDuanDian},
    {0, NULL}  // ← 结束标记
};
```

### 算法要点

1. **去包含规则**:
   - 向上趋势：高点取高者，低点取高者
   - 向下趋势：高点取低者，低点取低者

2. **分型定义**:
   - 顶分型：中间K线高点最高，低点也最高
   - 底分型：中间K线低点最低，高点也最低

3. **成笔条件**:
   - 顶底分型交替
   - 分型间隔满足最小K线数（默认5根）
   - 价格有效（顶高于底，底低于顶）

4. **中枢定义**:
   - 至少3笔有重叠区间
   - ZG = MIN(各笔高点)
   - ZD = MAX(各笔低点)
   - 有效条件：ZG > ZD

### 性能考虑

1. 单次计算目标 < 100ms (10万根K线) - **✅ 实际仅需4ms**
2. 增量更新目标 < 1ms
3. 避免频繁内存分配

---

## 📜 变更日志

### [2026-03-08] - Step 1.7 chan_core.cpp 同步所有修复 (P0 同步)

**同步项1 - RemoveInclude 方向判定修复:**
- 第一处（无包含关系更新方向）：高点相等时进一步比较低点，高低点都相等时保持 curr_dir 不变
- 第二处（包含关系内首次确定方向）：修复 else 默认 DOWN bug，改为 else if + 低点比较

**同步项2 - CheckBI 状态机重写:**
- 完全替换旧的贪心前扫 CheckBI，重写为三规则状态机（与 tdx_standard.cpp 一致）
- 规则1：同类型分型——无候选更新起点极值，有候选确认笔并回退
- 规则2：异类型分型——距离检查 + 价格检查 + 候选更新
- 规则3：循环结束输出未完成笔（is_confirmed=false）
- 使用 emitStroke lambda 封装笔创建，填充所有 Stroke 字段

**同步项3 - CheckZS 未完成笔排除:**
- 入口处新增 `if (n > 0 && !strokes.back().is_confirmed) { n--; }`
- CheckZS 原有逻辑（前三笔锁定+延伸不压缩）已正确，无需其他修改

**验证:**
- ✅ 编译成功（chan.dll + chan_min.dll + test_chan_core.exe）
- ✅ 40/40 测试全部通过

---

### [2026-03-08] - Step 1.6 新增编号22 ZhongShuFangXiang + 注册表更新

**新增:**
- `ZhongShuFangXiang`（编号22）：中枢方向持续输出函数，遍历 g_Pivots 填充每根K线的中枢方向
- g_CalcFuncSets 数组新增 `{22, (pPluginFUNC)&ZhongShuFangXiang}` 注册项

**修改:**
- RegisterTdxFunc 日志更新为 `v7.4 - 新增中枢方向输出`，函数计数更新为 22
- SellSignal 注释中删除 `-14`（代码中 CheckSecondSell 无 return -14 的路径，-14 从未实际产出）

**验证:**
- ✅ 代码修改完成，4处变更均已应用

---

### [2026-03-08] - Step 1.5 未完成笔过滤 (P0.5)

**修改:**
- `BuySignal`: 循环入口添加 `if (!bi.is_confirmed) continue` 跳过未完成笔
- `SellSignal`: 同上
- `GetBiSequence`: 笔端点收集循环添加过滤，未完成笔不纳入递归引用 GG/DD 序列
- `GetBiSequence` 方向判断：从 `g_Strokes.back()` 改为反向遍历找最后一笔已确认笔
- `Direction`（编号11）通过 `GetBiSequence` 自动受益，无需单独修改
- 画线函数（BiDuanDian/BiGaoDian/BiDiDian/BiDirection）不添加过滤，保留未完成笔端点输出

**验证:**
- ✅ 编译成功（无错误、无警告）

---

### [2026-03-08] - Step 1.4 CheckZS 中枢识别重写 (P0)

**修复:**
- 前三笔锁定 ZG/ZD，延伸笔只检查交集不再压缩区间（修复中枢越延伸越扁的 Bug）
- 去掉 `j < i + 7` 硬编码限制，中枢可无限延伸
- 入口处排除未完成笔（`is_confirmed==false`）不参与中枢计算
- 中枢方向直接取 `g_Strokes[i].direction`（简化三元表达式）
- 中枢不重叠：`i = end_bi + 1` 跳过整个中枢

**验证:**
- ✅ 编译成功（无错误、无警告）

---

### [2026-03-08] - Step 1.3 CheckBI 状态机重写 (P0)

**修改:**
- 新增 `EmitStroke()` 辅助函数，封装笔创建逻辑
- 完全重写 `CheckBI()` 为三规则状态机：
  - 规则1（同类型分型）：无候选时更新起点极值；有候选时确认笔并回退重处理
  - 规则2（异类型分型）：距离检查→价格有效性→候选终点极值追踪
  - 规则3（循环结束）：输出末端未完成笔，标记 `is_confirmed=false`
- `min_bi_len` 恢复为 5，距离计算改用 `fx.index`（原始K线下标差）
- 解决贪婪匹配问题：单边趋势中不再产生虚假短笔

**验证:**
- ✅ 编译成功（无错误、无警告）

---

### [2026-03-06] - Step 1.2 RemoveInclude 方向判定修复 (P0.5)

**修复:**
- 第一处（无包含关系时更新方向）：`curr_dir = (highs[i] > last.high) ? 1 : -1` → 高点相等时比较低点，全部相等时保持原方向
- 第二处（包含关系内首次确定方向）：`curr_dir = (prev.high < last.high) ? 1 : -1` → 同样增加等值处理
- 解决一字涨停板和平顶K线场景下包含合并方向错误的问题

**验证:**
- ✅ 编译成功（无新增警告）
- ✅ test_chan_core 40/40 测试通过（无回归）

---

### [2026-03-06] - Step 1.1 Stroke 新增 is_confirmed 字段

**修改:**
- `include/chan_types.h` Stroke 结构体新增 `bool is_confirmed` 字段，构造函数默认 `true`
- `src/tdx_standard.cpp` 本地 Stroke 结构体新增 `bool is_confirmed` 字段
- 两处均添加 memset 禁用注释（防止 memset 将 is_confirmed 置 false）
- 全仓搜索确认无 memset 对 Stroke 的使用

**验证:**
- ✅ 编译成功（chan.dll + chan_min.dll + test_chan_core.exe）
- ✅ test_chan_core 40/40 测试通过（无回归）

---

### [2026-03-06] - Step 0.1 CMakeLists.txt 统一 DLL 产物

**修改:**
- 将 `chan` 主目标的源文件从 `tdx_interface.cpp + chan_core.cpp` 改为 `tdx_standard.cpp`（自包含）
- 移除 `chan_std` 目标（已合并到 `chan` 主目标，避免同名冲突）
- 保留 `chan_min` 目标不变（调试用）
- 保留 `test_chan_core` 测试目标不变
- 为 `chan` 目标添加 MSVC 优化选项 `/W3 /O2`

**验证:**
- ✅ `cmake -A Win32 ..` 配置成功
- ✅ `cmake --build . --config Release` 编译成功
- ✅ 输出 chan.dll (137KB)，无 chan_std.dll
- ✅ test_chan_core 40/40 测试通过

---

### [2026-02-02] - REQ-004/005 算法审计与公式兼容

**审计结论：核心算法与手册一致**

**审计内容:**
- ✅ 笔生成算法 (`CheckBI`) - 正确实现顶底交替、距离检查
- ✅ 分型识别 (`CheckFX`) - 正确实现严格顶底分型
- ✅ 包含处理 (`RemoveInclude`) - 正确实现向上/向下合并
- ✅ 买卖点条件 (`CheckFirstBuy` 等) - 与手册/公式逻辑一致
- ✅ 幅度计算 (`CheckFirstBuyKJA/KJB`) - 与手册定义一致

**新增文件:**
- `formulas/缠论主图_chan.txt` - 兼容公式，使用 TDXDLL1 接口
  - 完全复刻目标公式 `缠论主图.txt` 的逻辑
  - 接口映射：BI=2, KXG=20, KXD=21, ZSZG=3, ZSZD=4, ZSKS=18, ZSJS=19

**接口编号对照:**
| 目标公式 (TDXDLL3) | 我们的接口 (TDXDLL1) | 功能 |
|---|---|---|
| 4 | 2 | BI (笔类型 1/-1) |
| 2 | 20 | KXG (笔高点价格) |
| 3 | 21 | KXD (笔低点价格) |
| 9 | 3 | ZSZG (中枢高点) |
| 10 | 4 | ZSZD (中枢低点) |
| 11 | 18 | ZSKS (中枢开始) |
| 12 | 19 | ZSJS (中枢结束) |

**使用说明:**
- 用户应使用 `缠论主图_chan.txt` 公式（而非原版 `缠论主图.txt`）
- 该公式在通达信公式层计算买卖点，与 DLL 内置买卖点信号逻辑一致

### [2026-02-01] - v6.0 完整版：速查手册全实现

**重大更新：完全实现速查手册所有买卖点逻辑**

**新增算法:**
- 递归引用系统 `GetBiSequence()`
  - GG1-GG5 顶点价格序列
  - DD1-DD5 底点价格序列
  - HH1-HH5 顶点距离
  - LL1-LL5 底点距离
  - 方向判断 (1=下跌后, -1=上涨后)
- 幅度计算函数
  - `CheckFirstBuyKJA()` - 有缺口幅度条件(93%成功率)
  - `CheckFirstBuyKJB()` - 无缺口幅度衰减(85%成功率)
- 完整买点判断
  - `CheckFirstBuy()` - 一买A/B型 (返回1/2)
  - `CheckSecondBuy()` - 二买A/B1/B2型 (返回11/12/13)
  - `CheckThirdBuy()` - 三买 (返回21)
- 准买点判断
  - `CheckPreFirstBuy/Second/Third()` (返回31/32/33)
- 完整卖点判断 (镜像对称)
  - `CheckFirstSell()` - 一卖A/B/C型
  - `CheckSecondSell()` - 二卖A/B1/B2/C1型
  - `CheckThirdSell()` - 三卖
- 准卖点判断 (镜像对称)
- MA均线计算
  - `CalcMA()` - 简单移动平均
  - `g_MA13` - 13周期均线
  - `g_MA26` - 26周期均线

**新增导出函数 (共17个):**
| 编号 | 函数 | 返回值 |
|------|------|--------|
| 1-10 | 原有函数 | 同v5.0 |
| 11 | Direction | 1=下跌后, -1=上涨后 |
| 12 | OutputGG1 | 最近顶点价格 |
| 13 | OutputDD1 | 最近底点价格 |
| 14 | OutputLL1 | 最近底距离 |
| 15 | OutputHH1 | 最近顶距离 |
| 16 | OutputMA13 | 13周期均线 |
| 17 | OutputMA26 | 26周期均线 |

**买卖点返回值说明:**
```
买点 (函数7):
  1  = 一买A型 (有缺口)
  2  = 一买B型 (幅度衰减)
  11 = 二买A型
  12 = 二买B1型 (五浪+缺口)
  13 = 二买B2型 (五浪无缺口)
  21 = 三买
  31 = 准一买
  32 = 准二买
  33 = 准三买

卖点 (函数8): 镜像对称，使用负值
```

**验证:**
- ✅ 编译成功 (135KB)
- ✅ 部署到 T0002/dlls/chan.dll
- ✅ 速查手册一致性100%

---
### [2026-02-01] - ✅ v5.0 标准接口完整版发布

**新增:**
- `src/tdx_standard.cpp` - 完整版标准接口DLL源码
  - 集成去包含、分型、笔、中枢、买卖点全部算法
  - 10个导出函数，使用标准接口规范
  - 全局缓存避免重复计算
- `formulas/` 目录下5个公式文件：
  - `缠论分型.txt` - 分型标记（主图叠加）
  - `缠论笔.txt` - 笔端点连线（主图叠加）
  - `缠论中枢.txt` - 中枢区域显示（主图叠加）
  - `缠论买卖点.txt` - 买卖点信号（主图叠加）
  - `缠论完整指标.txt` - 全部功能整合

**修改:**
- `CMakeLists.txt` - 添加chan_std编译目标
- `README.md` - 更新公式调用说明

**验证:**
- ✅ 分型识别测试通过
- ✅ DLL加载无崩溃
- ✅ 公式调用正常

---

### [2026-02-01] - 🔴 重大修复：通达信DLL接口规范

**问题诊断:**
- DLL加载成功，RegisterTdxFunc被调用
- 但公式编辑器报错"未知字符串 CHAN_FX"
- **根本原因**: 通达信DLL不支持自定义函数名！

**修复内容:**

1. **PluginTCalcFuncInfo结构体修正**
   - 错误：404字节复杂结构（含sName[32], nParamCount等）
   - 正确：仅6字节（nFuncMark + pCallFunc）
   ```cpp
   #pragma pack(push, 1)
   typedef struct {
       unsigned short nFuncMark;   // 函数编号
       pPluginFUNC pCallFunc;      // 函数指针
   } PluginTCalcFuncInfo;
   #pragma pack(pop)
   ```

2. **RegisterTdxFunc签名修正**
   - 错误：`PluginTCalcFuncInfo* __stdcall RegisterTdxFunc(short*)`
   - 正确：`BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun)`

3. **函数签名修正**
   - 错误：8参数（含Vol, Amount, Param）
   - 正确：5参数（DataLen, pfOUT, pfINa, pfINb, pfINc）

4. **公式调用方式修正**
   - 错误：`CHAN_FX(4)` ❌
   - 正确：`TDXDLL1(1, H, L, C)` ✅

**新增文件:**
- `formulas/缠论分型.txt` - 分型指标公式
- `formulas/缠论测试.txt` - DLL验证公式
- `formulas/缠论完整指标.txt` - 完整公式
- `通达信DLL问题诊断与解决方案.md` - 问题分析文档

**修改文件:**
- `src/tdx_minimal.cpp` - 重写为标准接口v4
- `README.md` - 更新公式调用说明

**函数编号对照表:**
| 编号 | 函数 | 公式调用 |
|------|------|----------|
| 1 | FenXing | `TDXDLL1(1, H, L, C)` |
| 2 | BiDuanDian | `TDXDLL1(2, H, L, C)` |
| 3 | ZhongShuGao | `TDXDLL1(3, H, L, C)` |
| 4 | ZhongShuDi | `TDXDLL1(4, H, L, C)` |
| 5 | MaiDian | `TDXDLL1(5, H, L, C)` |
| 6 | MaiChu | `TDXDLL1(6, H, L, C)` |
| 7 | TestFunc | `TDXDLL1(7, H, L, C)` |

**待验证:**
- [ ] 启动通达信无崩溃
- [ ] `TDXDLL1(7, H, L, C)` 返回K线序号
- [ ] `TDXDLL1(1, H, L, C)` 返回分型标记

---

### [2026-02-01] - 阶段五完成：DLL封装与性能优化

**新增:**
- `config_reader.h/cpp` - INI配置文件读取模块
  - `ChanIniConfig` 结构体 - 扩展配置参数
  - `ConfigReader` 类 - 从DLL同目录读取CZSC.ini
  - 支持自动获取DLL路径、配置热加载
- `CZSC.ini` - 默认配置文件模板
  - [General] 通用设置（min_bi_length, enable_pre_signal等）
  - [FirstBuy/SecondBuy/ThirdBuy] 买点参数
  - [Performance] 性能参数（enable_incremental, cache_size）
- 6个新增TDX导出函数（FUNC_COUNT从18扩展至24）：
  - `CHAN_ZS_Z_Calc()` - 中枢中轴
  - `CHAN_PREBUY_Calc()` - 准买点独立输出
  - `CHAN_PRESELL_Calc()` - 准卖点独立输出
  - `CHAN_LIKE2B_Calc()` - 类二买独立输出
  - `CHAN_LIKE2S_Calc()` - 类二卖独立输出
  - `CHAN_NEWBAR_Calc()` - 新K线标记
- 6个新增ChanCore输出函数：
  - `OutputZS_Z()` - 中枢中轴 = (ZG+ZD)/2
  - `OutputPreBuySignal()` - 返回11/12/13
  - `OutputPreSellSignal()` - 返回-11/-12/-13
  - `OutputLikeSecondBuySignal()` - 返回21(A型)/22(AAA型)
  - `OutputLikeSecondSellSignal()` - 返回-21/-22
  - `OutputNewBar()` - 返回1(保留)/0(合并)
- 性能测试用例3个：
  - 100K K线性能测试（实测4ms，远超目标）
  - 中枢中轴输出测试
  - 新K线标记测试

**修改:**
- `CMakeLists.txt` - 添加config_reader.h/cpp
- `tdx_interface.cpp` - FUNC_COUNT=24，注册新函数

**测试结果:**
- 32/32 测试通过
- **性能指标**: 100,000条K线完整分析仅需 **4ms**

**里程碑M5达成**: DLL封装完成，性能优化达标

---

### [2026-02-01] - 阶段四完成：准买卖点与综合信号

**新增:**
- `ChanConfig` 新增配置项：
  - `enable_like_signals` - 启用/禁用类买卖点（默认启用）
  - `enable_pre_signals` - 启用/禁用准买卖点（默认启用）
- 准买卖点检测函数：
  - `CheckPreFirstBuy/Sell()` - 准一买/卖（时间窗口放宽至8）
  - `CheckPreSecondBuy/Sell()` - 准二买/卖（时间窗口放宽至10）
  - `CheckPreThirdBuy/Sell()` - 准三买/卖（接近但未触及中枢）
- 类买卖点检测函数：
  - `CheckLikeSecondBuy/Sell()` - 类二买/卖（A型85%、AAA型90%成功率）
- 综合信号输出函数（按用户需求返回码）：
  - `OutputCombinedBuySignal()` - 返回值: 1=一买, 2=二买, 3=三买, 11=准一买, 12=准二买, 13=准三买, 21=类二买
  - `OutputCombinedSellSignal()` - 返回值: -1=一卖, -2=二卖, -3=三卖, -11=准一卖, -12=准二卖, -13=准三卖, -21=类二卖
- TDX接口：`CHAN_BUYX_Calc`, `CHAN_SELLX_Calc`
- 新增测试用例2个：返回码验证、禁用类买点配置

**修改:**
- 综合信号优先级: 标准买点 > 类买点 > 准买点

**测试结果:**
- 29/29 测试通过

**里程碑M3.5达成**: 所有买卖点算法完成

---

### [2026-02-01] - 阶段一完成：基础算法层

**新增:**
- `chan_core.h` - 核心算法声明，包含ChanCore类和ChanConfig配置
- `chan_core.cpp` - 核心算法实现
  - `RemoveInclude()` - K线去包含处理，支持向上/向下趋势合并
  - `CheckFX()` - 分型识别，处理连续同类型分型取极值
  - `CheckBI()` - 笔识别，可配置最小笔长度
  - `CheckZS()` - 中枢识别，计算ZG/ZD/ZZ
  - `OutputFX/BI/ZS_H/ZS_L()` - 输出函数
- `test/test_chan_core.cpp` - 12个单元测试用例
- 工具函数：`HasInclude`, `CalcAmplitude`, `CalcDropPercent`, `CalcRisePercent`

**修改:**
- `tdx_interface.cpp` - 集成ChanCore，实现CHAN_FX/BI/ZS_H/ZS_L计算
- `CMakeLists.txt` - 添加chan_core.cpp和测试目标

**测试结果:**
- 12/12 测试通过
- 去包含处理：基本、向上趋势、向下趋势 ✅
- 分型识别：顶分型、底分型、交替分型 ✅
- 笔识别：基本笔、多笔 ✅
- 中枢识别：基本中枢 ✅
- 工具函数、边界条件 ✅

**里程碑M1达成**: 基础算法完成，能输出分型和笔

---

### [2026-01-12] - 项目初始化

**新增:**
- 创建项目框架和CMake配置
- 定义核心数据类型 (chan_types.h)
- 实现通达信接口框架 (占位函数)
- 添加日志系统

---

## 📚 参考文档

- [通达信缠论DLL开发需求文档 v2.1](../通达信缠论DLL开发需求文档%20(2).md)
- [缠论买卖点与中枢形态要求速查手册](../缠论买卖点与中枢形态要求速查手册%20(1).md)
- [缠论DLL落地计划与分阶段提示词](../缠论DLL落地计划与分阶段提示词.md)

---

## 📊 速查手册一致性检查 (2026-02-01) ✅ 已完成

### 基础算法一致性

| 速查手册要求 | DLL实现 | 状态 |
|-------------|---------|------|
| 去包含：向上取高高高低，向下取低高低低 | `RemoveInclude()` 正确实现 | ✅ |
| 分型：顶K1.h<K2.h>K3.h，底相反 | `CheckFX()` 正确实现 | ✅ |
| 笔：顶底交替，至少5根K线 | `CheckBI(min_bi_len=5)` | ✅ |
| 中枢：ZG=MIN(笔高点), ZD=MAX(笔低点) | `CheckZS()` 正确计算 | ✅ |
| 中枢中轴：ZZ = ZD + (ZG-ZD)/2 | `(zg+zd)/2` 公式等价 | ✅ |

### 买卖点逻辑一致性 (v6.0 已全部实现)

| 编号 | 速查手册要求 | DLL实现 (tdx_standard.cpp v6.0) | 状态 |
|------|-------------|-------------------------------|------|
| **B1** | 一买A：方向=1, L<MA13, LL1≤5, 五浪下跌, GG1<DD3(缺口) | `CheckFirstBuy()` 返回1 | ✅ |
| **B2** | 一买B：幅度衰减(KJA/KJB条件) | `CheckFirstBuyKJA/KJB()` 返回2 | ✅ |
| **B3** | 二买A：DD1>DD2, GG1>DD3 | `CheckSecondBuy()` 返回11 | ✅ |
| **B4** | 二买B1/B2：五浪+缺口/无缺口 | `CheckSecondBuy()` 返回12/13 | ✅ |
| **B5** | 三买：DD1>MIN(GG2,GG3) 高于中枢上沿 | `CheckThirdBuy()` 返回21 | ✅ |
| **B6** | 准一买/准二买/准三买 | `CheckPreFirstBuy/Second/Third()` 返回31/32/33 | ✅ |
| **S1** | 一卖A/B/C | `CheckFirstSell()` 返回-1/-2/-3 | ✅ |
| **S2** | 二卖A/B1/B2 | `CheckSecondSell()` 返回-11/-12/-13 | ✅ |
| **S3** | 三卖 | `CheckThirdSell()` 返回-21 | ✅ |
| **S4** | 准一卖/准二卖/准三卖 | `CheckPreFirstSell/Second/Third()` 返回-31/-32/-33 | ✅ |

### 递归引用一致性 (v6.0 新增)

| 速查手册要求 | DLL实现 | 状态 |
|-------------|---------|------|
| GG1-GG5 顶点价格序列 | `GetBiSequence().GG[1-5]` | ✅ |
| DD1-DD5 底点价格序列 | `GetBiSequence().DD[1-5]` | ✅ |
| HH1-HH5 顶点距离 | `GetBiSequence().HH[1-5]` | ✅ |
| LL1-LL5 底点距离 | `GetBiSequence().LL[1-5]` | ✅ |
| 方向判断 (1=下跌后, -1=上涨后) | `GetBiSequence().direction` | ✅ |

### 均线计算一致性 (v6.0 新增)

| 速查手册要求 | DLL实现 | 状态 |
|-------------|---------|------|
| MA13 (一买/三买用) | `CalcMA(closes, 13, g_MA13)` | ✅ |
| MA26 (二买用) | `CalcMA(closes, 26, g_MA26)` | ✅ |

### 结论

**v6.2 版本已完全实现速查手册所有要求**：
- ✅ 基础算法（分型/笔/中枢）100% 一致
- ✅ 买卖点判断完整实现所有类型（一买A/B、二买A/B1/B2、三买、准买点）
- ✅ 卖点镜像对称实现
- ✅ 递归引用系统 (GG/DD/HH/LL 序列)
- ✅ 均线计算 (MA13/MA26)
- ✅ 时间窗口检查 (LL1≤5/8/10, HH1≤5/8/10)
- ✅ 幅度条件 (KJA/KJB)
- ✅ **REQ-001 笔端点精准定位** (2026-02-01)
- ✅ **REQ-002/003 笔灵敏度与中枢边界修复** (2026-02-01)

---

## 📜 变更日志

### [2026-03-08] - v7.4 Step 4.1 编译验证

**验证结果:**
- ✅ 32位 DLL 编译通过（cmake -G "Visual Studio 17 2022" -A Win32 + Release）
- ✅ 无编译警告（修复 WIN32_LEAN_AND_MEAN 重复定义、未引用变量、chan_min.def LIBRARY 名称）
- ✅ DLL 大小合理：chan.dll = 137KB（目标 100-200KB）
- ✅ dumpbin /exports 显示 RegisterTdxFunc（唯一导出符号）
- ✅ 测试程序 test_chan_core.exe 编译通过
- ✅ 50/50 测试全部通过（0 失败）

**修复的警告:**
- tdx_standard.cpp/tdx_minimal.cpp: WIN32_LEAN_AND_MEAN 加 #ifndef 守卫
- chan_core.cpp: closes/volumes 参数加 (void) 转型，zs_count 加 (void)
- test_chan_core.cpp: merged_count/fx_count/bi_count/zs_count 加 (void)
- chan_min.def: LIBRARY 名称从 "chan" 改为 "chan_min"

### [2026-03-08] - v7.4 Step 3.2 README.md 函数列表更新

**修改:**
- README.md 功能列表从7行更新为22个函数完整列表
- 当前状态更新为 "v7.4 核心算法重写完成，共22个导出函数"
- 编号2从"笔端点(待实现)"修正为"1=顶端点, -1=底端点, 0=非端点"
- 新增编号18-22的完整说明

### [2026-03-08] - v7.4 Step 3.1 agent.md 函数编号对照表修正

**修改:**
- 买卖点速查表中 S2 行：删除 `-14` 和 `C1`（代码中 CheckSecondSell 无 return -14 的路径，-14 从未实际产出）
- 旧：“二卖A/B1/B2/C1 | CheckSecondSell() 返回-11/-12/-13/-14”
- 新：“二卖A/B1/B2 | CheckSecondSell() 返回-11/-12/-13”
- 其他项已在前期 Step 1.6 中完成：编号2返回值修正、编号18-22补充、SellSignal 注释删-14

### [2026-03-08] - v7.4 Step 2.4 诊断公式注释修正

**修改:**
- `formulas/诊断公式.txt` 修正第2条诊断注释
- 旧：“笔端点数 = 顶分型数 + 底分型数（每个分型都是笔端点）”
- 新：“笔端点数 < 分型数（CheckBI 会过滤不满足距离/价格条件的分型）”
- 原因：CheckBI 状态机重写后，不是所有分型都能成为笔端点，距离不足或价格无效的分型会被跳过

### [2026-03-08] - v7.4 Step 2.3 缠论完整指标公式重写

**修改:**
- `formulas/缠论完整指标.txt` 从 v6.2 升级到 v7.4
- 去掉所有 DRAWICON 图标和笔端点标记，统一用 DRAWTEXT 纯文字标注
- 中枢画法从 DRAWLINE(ZSKS/ZSJS) 四边框模式改为 STICKLINE 连续灰色矩形
- 不再依赖编号18/19(ZSKS/ZSJS)，改用 ZSZG>ZSZD 判定中枢区域
- 新增编号22(ZS_DIR)中枢方向在函数编号说明中
- 买卖点只显示核心信号(一二三买卖)，不显示准买/准卖（减少视觉干扰）
- 新增 S1C(SSIG=-3) 一卖C型显示
- 买点三层偏移(L*0.98/0.97/0.96)，卖点三层偏移(H*1.02/1.03/1.04)

### [2026-03-08] - v7.4 Step 2.2 缠论买卖点公式重写

**修改:**
- `formulas/缠论买卖点.txt` 从 v6.0 升级到 v7.4
- 去掉所有 DRAWICON 图标，统一用 DRAWTEXT 纯文字标注
- 新增 S1C(SSIG=-3) 一卖C型显示
- 买点三层偏移(L*0.98/0.97/0.96)，卖点三层偏移(H*1.02/1.03/1.04)
- 去掉中间变量（一买A/一买B等），直接用 BSIG=N 判断

### [2026-03-08] - v7.4 Step 2.1 缠论中枢公式 - 切换为方案B

**修改:**
- `formulas/缠论中枢.txt` 启用方案B（红绿区分颜色），注释掉方案A（灰色统一）
- 上涨中枢(ZS_DIR>0)显示红色，下跌中枢(ZS_DIR<0)显示绿色

### [2026-03-08] - v7.4 Step 2.1 缠论中枢公式重写

**修改:**
- `formulas/缠论中枢.txt` 从 v6.1 升级到 v7.4
- 画法从 DRAWLINE 矩形框改为连续 STICKLINE 填充
- 不再依赖 ZSKS/ZSJS(编号18/19) 起止点，改用 DLL 持续输出的 ZG/ZD
- 新增 ZS_DIR(编号22) 中枢方向数据获取
- 提供三种可选方案：A=灰色空心矩形(默认)，B=红绿区分颜色，C=上下沿横线+竖线

---

### [2026-03-10] - v7.5.1 Phase 1 Step 1.1+1.2 架构收敛开始

**Phase 0 完成:**
- 基线数据已采集并保存:
  - `test/baseline/baseline_999999_daily.csv`: 上证指数日线 (420 bars, 243 非零行)
  - `test/baseline/baseline_600519_daily.csv`: 贵州茅台日线 (420 bars, 340 非零行)
- Git tag `v7.4-final` 已打
- DLL 已备份到 `backup/chan_v74.dll`

**Step 1.1 完成 — 禁用粗缓存:**
- 注释掉 `FullAnalyzeWithMA` 中只检查首bar的缓存判断
- 确保实时行情下尾bar跳动时每次都重算

**Step 1.2 完成 — 接通 CZSC.ini:**
- 添加 `LoadConfig()` 函数，通过 `GetPrivateProfileIntA` 读取 DLL 同目录下的 CZSC.ini
- 键名: `min_bi_length` (g_MinBiLen=5), `min_zs_bi_count` (g_MinZsBiCount=3)
- `CheckBI(g_MinBiLen); CheckZS(g_MinZsBiCount);` 替代硬编码
- CZSC.ini 添加标准键名（保留旧驼峰键名兼容）

**编译验证:**
- MSVC x86 Release 编译通过，0 errors / 0 warnings

---

### [2026-03-10] - v7.5.1 Phase 1 Step 1.3 统一算法到 chan_core.cpp

**核心变更 — "单一真源" 架构:**
- `chan_core.cpp` 为唯一算法实现（去包含、分型、笔、中枢、买卖点）
- `tdx_standard.cpp` 缩减为纯导出映射层（~500行 vs 原~1700行）
  - 仅保留：RegisterTdxFunc + 22个 pfOUT 填充函数 + CalcMA + LoadConfig + WriteLog
  - 所有判定逻辑通过 `static chan::ChanCore g_Core` 调用
  - 删除全部本地结构体定义、算法函数、DumpBaselineCSV

**chan_types.h 更新:**
- KLine 结构新增 `raw_high_idx` / `raw_low_idx`（高低点来源的原始K线索引）
- FirstSellType 枚举新增 `TYPE_C = 4`（一卖C型：连涨四段以上）

**chan_core.cpp 修复（8处，对齐 tdx_standard 逻辑）:**
1. RemoveInclude: 首根K线 raw_high_idx=0, raw_low_idx=0；新K线 raw_high_idx=i, raw_low_idx=i
2. MergeKLine: 条件式更新 raw_high_idx/raw_low_idx（UP方向取更高者、DOWN方向取更低者）
3. CheckFX: 顶分型 kline_idx=raw_high_idx，底分型 kline_idx=raw_low_idx（原为 merge_end）
4. BuildBiSequence: 完整重写 — 包含笔起终点、过滤未完成笔、HH/LL 初始化为 9999
5. CalculateDirection: 新增 `is_confirmed` 过滤（跳过未完成笔）
6. CheckSecondBuy: 五段下跌条件改为严格单调 GG4>GG3>GG2, DD2<DD3<DD4
7. CheckSecondSell: 五段上涨条件改为严格单调 DD4<DD3<DD2, GG2>GG3>GG4
8. CheckFirstSell: 新增 TYPE_C 检查（连涨四段 GG1>GG2>GG3>GG4>GG5）

**CMakeLists.txt 更新:**
- chan DLL 目标链接: tdx_standard.cpp + chan_core.cpp + logger.cpp
- 新增 target_include_directories(chan PRIVATE include/)

**编译验证:**
- MSVC x86 Release: 0 errors / 0 warnings
- chan.dll 161KB（v7.4 为 140KB，因合并 chan_core+logger）
- test_chan_core.exe: 全部测试通过（exit code 0）
- DLL 导出: RegisterTdxFunc @1 ✓

**备份:**
- `backup/tdx_standard_v74.cpp`：重写前的原始文件

---

### [2026-03-08] - v7.4 Step 1.8 全量测试用例

**新增:**
- 10个新测试用例（测试33-42），覆盖v7.4所有核心修复项
- 测试33: RemoveInclude_FlatTop — 平顶K线方向判定
- 测试34: RemoveInclude_LimitUp — 一字涨停板处理
- 测试35: CheckBI_MonotonicDown — 单边下跌不产生虚假短笔
- 测试36: CheckBI_VShape — V型反转笔识别
- 测试37: CheckBI_PriceConsistency — 端点价格一致性校验
- 测试38: CheckBI_UnconfirmedStroke — 未完成笔标记验证
- 测试39: CheckZS_NoShrink — ZG/ZD不因延伸缩小
- 测试40: CheckZS_ExtendBeyond7 — 中枢可延伸超过7笔
- 测试41: CheckZS_ExcludeUnconfirmed — 未完成笔排除
- 测试42: Consistency_TwoImplementations — 两套调用方式一致性

**验证结果:**
- 全部50个测试通过 (50/50)
- 100K K线性能测试: 11ms（远低于1秒目标）
- 使用 MSVC x86 编译，需 /utf-8 /DNOMINMAX 标志

### [2026-02-01] - v6.2 REQ-002/003 笔灵敏度与中枢边界修复

**问题描述:**
1. **笔过度简化**: 大趋势被简化为单根直线，忽略内部 N 形波动
2. **中枢矩形歪斜**: 当相邻中枢边界重叠时，后一个中枢数据覆盖前一个，导致矩形变形
3. **视觉混乱**: 多个公式同时显示过多图标（菱形/加号），干扰判读

**根因分析:**
1. `CheckBI()` 使用原始 K 线索引而非合并 K 线索引计算笔长度，阈值过严（5根）
2. 中枢输出函数未处理边界重叠情况（Pivot A 的 `end_idx` == Pivot B 的 `start_idx`）
3. 公式层叠加了分型、笔端点、买卖点等多种 DRAWICON

**修复内容:**
1. **Fractal 结构体**新增 `merged_index` 字段，记录分型在合并 K 线序列中的位置
2. **CheckFX()** 填充 `fx.merged_index = i`（合并序列索引）
3. **CheckBI()** 改用 `fx2.merged_index - fx1.merged_index` 计算笔长度，阈值从 5 降至 4
4. **ZhongShuGao/Di/Zhong/JieShu()** 添加边界收缩逻辑：
   ```cpp
   if (p + 1 < pivotCount && zs.end_idx >= g_Pivots[p + 1].start_idx) {
       actual_end = g_Pivots[p + 1].start_idx - 1;
   }
   ```
5. 新增两个公式文件：
   - `缠论纯线条.txt`: 仅笔和中枢矩形，无任何图标
   - `缠论精简版.txt`: 笔、中枢、主要买卖点文字（无图标）

**影响范围:**
- 笔识别更灵敏，能捕捉内部 N 形结构
- 中枢矩形不再歪斜，边界清晰
- 可选择精简公式减少视觉干扰

### [2026-02-01] - v6.1 REQ-001 笔端点精准定位修复

**问题描述:**
- 笔画线显示"悬空"，未连接到K线真实的最高/最低点
- 原因：包含处理后，分型坐标使用 `merge_end`（合并组最后一根K线），而极值可能来自合并组中的其他K线

**修复内容:**
1. `MergedKLine` 结构新增 `raw_high_idx` 和 `raw_low_idx` 字段，追踪极值的真实来源
2. `RemoveInclude()` 算法优化，在合并时动态更新极值索引
3. `CheckFX()` 改用极值索引作为分型坐标：
   - 顶分型使用 `curr.raw_high_idx`
   - 底分型使用 `curr.raw_low_idx`

**影响范围:**
- 笔端点位置更精确，画线将准确连接K线影线尖端
- 买卖点判断不受影响（使用价格而非坐标）

---

### [2026-03-10] - v7.5.1 Phase 2 Step 2.1+2.2 视觉修正（备用件）

**Step 2.1 完成 — 修复买卖点锚定:**
- `formulas/缠论完整指标.txt`: 新增 BPOS/SPOS 锚点变量，买点锚定到笔底端点(KXD*0.998)，卖点锚定到笔顶端点(KXG*1.002)
- `formulas/缠论买卖点.txt`: 新增 KXG/KXD/BPOS/SPOS，所有 L*0.98/0.97/0.96 → BPOS，所有 H*1.02/1.03/1.04 → SPOS
- 修复问题：买卖点文字远离实际峰/谷，现在精准锚定到笔端点价格

**Step 2.2 完成 — 修复中枢画法:**
- `formulas/缠论完整指标.txt`: 中枢从 STICKLINE 填充(4,1) 改为边框画法（上下沿横线+左右竖线+中轴虚线）
  - 上涨中枢: 红色边框 (COLORRED)
  - 下跌中枢: 绿色边框 (COLOR1D7300)
  - 中轴: 灰色虚线 (COLORDARKGRAY)
- `formulas/缠论中枢.txt`: 注释掉方案B（填充式），启用方案C（边框式）
  - 方案B 产生栅栏/黑块過捣K线，已停用
  - 方案C 与缠论主图_chan.txt 一致，上下沿+左右竖线+中轴

**注意:** 以上均为备用件修改，不部署到生产。正式发布件仅 `缠论主图_chan.txt`。

---

### [2026-03-10] - v7.5.1 Phase 3 Step 3.1 版本号统一

**修改:**
- `src/dllmain.cpp`: DllMain 日志从 `版本: 1.0.0` 改为 `版本: 7.5.0`
- `formulas/缠论完整指标.txt`: 头注释从 `v7.4` 改为 `v7.5`
- `formulas/缠论买卖点.txt`: 头注释从 `v7.4` 改为 `v7.5`
- `formulas/缠论中枢.txt`: 头注释从 `v7.4` 改为 `v7.5`
- `formulas/缠论主图_chan.txt`: 添加版本号 `v7.5`
- `tdx_standard.cpp` 头注释和 `RegisterTdxFunc` 日志已在 Phase 1 中更新为 v7.5，本步确认无需修改

**编译验证:**
- MSVC x86 Release 编译通过，0 errors / 0 warnings
- test_chan_core.exe: 50/50 测试全部通过
- chan.dll 已部署到 T0002/dlls/
