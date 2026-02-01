# 通达信DLL问题诊断与解决方案

**项目**: https://github.com/15002430024/chan_tdx_plugin
**日期**: 2026年2月1日

---

## 🔴 核心问题定位

### 问题现象
- ✅ DLL 已成功加载
- ✅ `RegisterTdxFunc` 被通达信调用（日志确认）
- ❌ 公式编辑器中 `CHAN_FX(4)` 报错"未知字符串"

### 🚨 根本原因：调用方式错误

**通达信DLL不支持自定义函数名！**

你在公式中使用：
```
FX:CHAN_FX(4);  ← ❌ 错误！通达信不认识这个名字
```

正确的调用方式：
```
FX:TDXDLL1(1, H, L, C);  ← ✅ 正确！用TDXDLL+序号
```

---

## 📚 通达信DLL调用规范

### 标准接口（官方规范）

```cpp
// 1. 函数签名（固定格式）
typedef void(*pPluginFUNC)(int DataLen, float* pfOUT, 
                           float* pfINa, float* pfINb, float* pfINc);

// 2. 注册结构
typedef struct tagPluginTCalcFuncInfo {
    unsigned short nFuncMark;   // 函数编号（1, 2, 3...）
    pPluginFUNC pCallFunc;      // 函数指针
} PluginTCalcFuncInfo;

// 3. 注册数组
PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1, (pPluginFUNC)&FenXing},      // 编号1 → 分型
    {2, (pPluginFUNC)&BiDuanDian},   // 编号2 → 笔端点
    {3, (pPluginFUNC)&ZhongShuGao},  // 编号3 → 中枢高
    {0, NULL}  // ← 必须以 {0, NULL} 结尾
};

// 4. 导出函数
extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    if (*pFun == NULL) {
        (*pFun) = g_CalcFuncSets;
        return TRUE;
    }
    return FALSE;
}
```

### 公式调用方式

```
{假设DLL绑定为第1号DLL}

{调用编号1的函数 → 分型}
FX:TDXDLL1(1, H, L, C);

{调用编号2的函数 → 笔端点}  
BI:TDXDLL1(2, H, L, C);

{调用编号3的函数 → 中枢高}
ZS_H:TDXDLL1(3, H, L, C);
```

**关键点**：
- `TDXDLL1` = 第1号DLL
- `TDXDLL2` = 第2号DLL
- 第一个参数 = `nFuncMark`（函数编号）
- 后三个参数 = 传给函数的数据数组

---

## 🔧 解决方案

### 方案一：修改公式调用方式（推荐，最快）

**步骤1**: 确认DLL绑定的是第几号

在通达信公式管理器 → DLL函数 → 查看你的 `chan.dll` 绑定的是 `TDXDLL1` 还是 `TDXDLL2` 等。

**步骤2**: 修改公式

假设绑定为第1号DLL，你的代码中 `g_CalcFuncSets` 定义为：
```cpp
PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1, (pPluginFUNC)&CHAN_FX_Func},     // 分型
    {2, (pPluginFUNC)&CHAN_BI_Func},     // 笔
    {3, (pPluginFUNC)&CHAN_DUAN_Func},   // 段
    {4, (pPluginFUNC)&CHAN_ZS_H_Func},   // 中枢高
    {5, (pPluginFUNC)&CHAN_ZS_L_Func},   // 中枢低
    {6, (pPluginFUNC)&CHAN_BUY_Func},    // 买点
    {7, (pPluginFUNC)&CHAN_SELL_Func},   // 卖点
    {0, NULL}
};
```

公式应该写成：
```
{分型标记 - 编号1}
FX:TDXDLL1(1, H, L, C);

{笔端点 - 编号2}
BI:TDXDLL1(2, H, L, C);

{线段端点 - 编号3}
DUAN:TDXDLL1(3, H, L, C);

{中枢高点 - 编号4}
ZS_H:TDXDLL1(4, H, L, C);

{中枢低点 - 编号5}
ZS_L:TDXDLL1(5, H, L, C);

{买点信号 - 编号6}
BUY:TDXDLL1(6, H, L, C);

{卖点信号 - 编号7}
SELL:TDXDLL1(7, H, L, C);
```

---

### 方案二：参考 louis-gg/tdxchanbi 的标准写法

```cpp
// TCalcFuncSets.cpp

#include "PluginTCalcFunc.h"

// 分型识别函数
void FenXing(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    // pfINa = HIGH 数组
    // pfINb = LOW 数组
    // pfINc = CLOSE 数组（或其他）
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = 0;  // 默认无分型
        // ... 分型识别逻辑
    }
}

// 注册数组 - 必须是全局变量
PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1, (pPluginFUNC)&FenXing},
    {0, NULL}  // ← 结束标记
};

// 导出函数
extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    if (*pFun == NULL) {
        (*pFun) = g_CalcFuncSets;
        return TRUE;
    }
    return FALSE;
}
```

---

### 方案三：参考 rust-chan 的公式写法

rust-chan 项目使用的公式调用：

```
{MODE参数控制功能}
MODE:=(1+BI_QK*4+CIGAO) * 1000 + PIVOT * 10;
M_VALUE:=100;
M_EDGE:=200;
M_SIG:=400;

{调用DLL - 注意是 TDXDLL2 因为绑定为第2号}
FRAC_X:=TDXDLL2(1, HIGH, LOW, MODE+M_EDGE);
POLE_VALUE:TDXDLL2(1, HIGH, LOW, MODE+M_VALUE);
SIG:TDXDLL2(2, HIGH, LOW, MODE+M_SIG);
```

**设计思路**：
- 用第三个参数 `pfINc` 传递模式控制码
- 不同模式码返回不同数据
- 减少函数数量，提高灵活性

---

## ✅ 快速验证步骤

### 1. 创建最小测试版本

```cpp
// tdx_test.cpp - 最小测试版本
#include <windows.h>

#pragma pack(push, 1)
typedef void(*pPluginFUNC)(int, float*, float*, float*, float*);
typedef struct {
    unsigned short nFuncMark;
    pPluginFUNC pCallFunc;
} PluginTCalcFuncInfo;
#pragma pack(pop)

// 测试函数：返回序号
void TestFunc(int DataLen, float* pfOUT, float* pfINa, float* pfINb, float* pfINc) {
    for (int i = 0; i < DataLen; i++) {
        pfOUT[i] = (float)i;  // 返回K线序号
    }
}

// 注册数组
PluginTCalcFuncInfo g_CalcFuncSets[] = {
    {1, (pPluginFUNC)&TestFunc},
    {0, NULL}
};

// 导出函数
extern "C" __declspec(dllexport) 
BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun) {
    if (*pFun == NULL) {
        (*pFun) = g_CalcFuncSets;
        return TRUE;
    }
    return FALSE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    return TRUE;
}
```

### 2. 编译命令

```batch
cl /LD /MT tdx_test.cpp /Fe:test.dll /link /DEF:test.def
```

### 3. test.def 文件

```
LIBRARY test
EXPORTS
    RegisterTdxFunc
```

### 4. 测试公式

```
TEST:TDXDLL1(1, H, L, C);
```

如果这个能工作，说明基础框架正确，问题在于你的实现细节。

---

## 🔍 检查清单

| 检查项 | 预期 | 你的代码 |
|--------|------|----------|
| 编译为 **Win32 (x86)** | ✅ | ？ |
| `#pragma pack(push, 1)` | ✅ | ？ |
| `g_CalcFuncSets` 以 `{0, NULL}` 结尾 | ✅ | ？ |
| `RegisterTdxFunc` 用 `extern "C"` 导出 | ✅ | ？ |
| .def 文件导出 `RegisterTdxFunc` | ✅ | ？ |
| 公式用 `TDXDLL1(N, ...)` 而非函数名 | ✅ | ❌ |

---

## 📋 总结

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| `CHAN_FX` 报错 | 通达信不支持自定义函数名 | 改用 `TDXDLL1(编号, H, L, C)` |

**你的README中的公式调用方式是错误的**：
```
FX:CHAN_FX(4);  ← ❌ 不可能工作
```

**正确写法**：
```
FX:TDXDLL1(1, H, L, C);  ← ✅ 第1号DLL的第1个函数
```

---

## 🚀 下一步行动

1. **立即验证**：修改通达信公式，把 `CHAN_FX(4)` 改成 `TDXDLL1(1, H, L, C)`
2. **更新文档**：修改 README.md 中的公式示例
3. **考虑设计**：是否需要像 rust-chan 那样用模式码控制功能，而不是注册大量函数
