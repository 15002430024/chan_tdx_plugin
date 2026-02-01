# 🚨 当前问题：通达信无法识别DLL注册的函数

## 问题概述

DLL 已成功加载，`RegisterTdxFunc` 已被通达信调用，但在公式编辑器中使用函数时报错：**"未知字符串 CHAN_FX"**

## 已确认的事实

### ✅ 工作正常的部分

1. **DLL 编译成功** - 32位 x86 DLL，83KB
2. **DLL 部署正确** - 位于 `T0002\dlls\chan.dll`
3. **dlls.ini 注册正确** - `band1=chan.dll`
4. **通达信启动正常** - 不再崩溃
5. **RegisterTdxFunc 被调用** - 日志已确认
6. **函数信息返回正确** - 日志显示 name=CHAN_FX, paramCount=0

### ❌ 不工作的部分

- 公式编辑器中输入 `CHAN_FX` / `CHAN_FX()` / `CHAN_FX;` 均提示 **"未知字符串 CHAN_FX"**

## 调试日志

```
[10:23:45] === DLL_PROCESS_ATTACH ===
[10:23:45] RegisterTdxFunc 被调用! sizeof(PluginTCalcFuncInfo)=404
[10:23:45] 函数已初始化: name=CHAN_FX, paramCount=0, calcFunc=0x10001234
```

## 当前代码 (tdx_minimal.cpp)

```cpp
typedef void (__stdcall *PluginTCalcFunc)(
    int nCount, 
    float* pOut, 
    float* pHigh, 
    float* pLow, 
    float* pClose, 
    float* pVol, 
    float* pAmount, 
    float* pParam
);

#pragma pack(push, 4)
struct PluginTCalcFuncInfo {
    unsigned short nFuncMark;   // 2 bytes, 标识 = 0x0001
    char    sName[32];          // 32 bytes, 函数名
    unsigned char nParamCount;  // 1 byte, 参数个数
    unsigned char nParamType[8];// 8 bytes, 参数类型
    char    sParamName[8][32];  // 256 bytes, 参数名
    float   fParamMin[8];       // 32 bytes
    float   fParamMax[8];       // 32 bytes
    float   fParamDef[8];       // 32 bytes
    PluginTCalcFunc pCalcFunc;  // 4 bytes, 函数指针
};
#pragma pack(pop)

extern "C" __declspec(dllexport) PluginTCalcFuncInfo* __stdcall RegisterTdxFunc(short* pnFuncNum)
{
    // ... 初始化 g_FuncInfo 数组 ...
    if (pnFuncNum != NULL) {
        *pnFuncNum = 1;
    }
    return g_FuncInfo;  // 返回函数信息数组指针
}
```

## 已尝试的不同方案

### 1. RegisterTdxFunc 签名变体

```cpp
// 尝试1: 双指针 (崩溃)
void __stdcall RegisterTdxFunc(PluginTCalcFuncInfo** ppInfo, int* pCount)

// 尝试2: 返回指针 + 数量参数 (当前使用，不崩溃但函数不识别)
PluginTCalcFuncInfo* __stdcall RegisterTdxFunc(short* pnFuncNum)

// 尝试3: 无参数返回指针 (未测试)
PluginTCalcFuncInfo* __stdcall RegisterTdxFunc(void)
```

### 2. 结构体对齐方式

```cpp
// 尝试1: #pragma pack(push, 1) → sizeof = 399
// 尝试2: #pragma pack(push, 4) → sizeof = 404 (当前)
// 尝试3: 无 pack → 默认对齐
```

### 3. nFuncMark 值

```cpp
// 尝试: 0x0001, 0x0000, 1, 0
```

## 🔍 请帮助分析

请对比以下开源通达信DLL插件项目，找出结构体定义或接口签名的差异：

### 参考项目

1. **CZSC 缠论库** - https://github.com/waditu/czsc
2. **通达信公式DLL** - 搜索 "通达信 DLL 插件 PluginTCalcFuncInfo"
3. **股票分析DLL** - 搜索 "tdx dll plugin RegisterTdxFunc"

### 需要确认的问题

1. **PluginTCalcFuncInfo 结构体布局是否正确？**
   - 字段顺序？
   - 字段大小？（特别是 sName、sParamName 数组大小）
   - 对齐方式？

2. **RegisterTdxFunc 签名是否正确？**
   - 返回类型？
   - 参数类型？
   - 调用约定（__stdcall vs __cdecl）？

3. **是否需要其他导出函数？**
   - 如 `UnRegisterTdxFunc`？
   - 或其他初始化函数？

4. **结束标记的写法？**
   - 是 `nFuncMark = 0` 还是整个结构体置零？
   - 还是用 NULL 指针？

## 参考：已知工作的 DLL

通达信自带的 `T0002\dlls\cl.dll` 和 `clxg.dll`（23KB）可以正常工作。

## 文件结构

```
chan_tdx_plugin/
├── src/
│   ├── tdx_minimal.cpp     ← 当前最小测试代码
│   ├── tdx_interface.cpp   ← 完整版接口代码
│   └── chan_core.cpp       ← 缠论核心算法 (40个测试通过)
├── include/
│   ├── tdx_interface.h
│   └── chan_core.h
├── chan_min.def            ← 导出定义文件
├── CMakeLists.txt
└── ISSUE.md                ← 本文件
```

## 构建命令

```powershell
# 使用 MSVC x86 编译
cd build
cmake .. -G "Visual Studio 17 2022" -A Win32
cmake --build . --config Release --target chan_min
```
