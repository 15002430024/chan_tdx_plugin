# 缠论通达信DLL插件

基于缠论理论的通达信技术分析插件。

## ✅ 当前状态

**DLL接口已修复，遵循通达信标准规范。**

## 功能列表

| 函数编号 | 功能 | 公式调用 | 输出说明 |
|----------|------|----------|----------|
| 1 | 分型标记 | `TDXDLL1(1, H, L, C)` | 1=顶分型, -1=底分型, 0=无 |
| 2 | 笔端点 | `TDXDLL1(2, H, L, C)` | 笔端点价格（待实现） |
| 3 | 中枢高点 | `TDXDLL1(3, H, L, C)` | 中枢ZG值（待实现） |
| 4 | 中枢低点 | `TDXDLL1(4, H, L, C)` | 中枢ZD值（待实现） |
| 5 | 买点信号 | `TDXDLL1(5, H, L, C)` | 买点类型（待实现） |
| 6 | 卖点信号 | `TDXDLL1(6, H, L, C)` | 卖点类型（待实现） |
| 7 | 测试函数 | `TDXDLL1(7, H, L, C)` | 返回K线序号（验证DLL） |

## 构建方法

### 环境要求

- Visual Studio 2019 或 2022
- CMake 3.15+
- Windows SDK

### 构建步骤

```batch
# 1. 创建构建目录
mkdir build
cd build

# 2. 生成项目 (必须使用Win32)
cmake -G "Visual Studio 17 2022" -A Win32 ..

# 3. 编译
cmake --build . --config Release
```

### 输出文件

编译成功后，在 `build/bin/Release/` 目录下生成 `chan.dll`。

## 安装方法

1. 将 `chan.dll` 复制到通达信安装目录的 `T0002/dlls/` 文件夹
2. 修改 `T0002/dlls/dlls.ini`，添加一行：
   ```ini
   [BAND]
   band1=chan.dll
   ```
3. 重启通达信

## 公式调用示例

**⚠️ 重要：通达信DLL不支持自定义函数名！必须使用 `TDXDLL + 序号` 的方式调用。**

```
{==== 分型指标 ====}
{编号1: 分型识别，返回 1=顶分型, -1=底分型, 0=无}
FX:TDXDLL1(1, H, L, C);

{顶底分型标记}
DRAWICON(FX=1, H, 1);   {顶分型画向下箭头}
DRAWICON(FX=-1, L, 2);  {底分型画向上箭头}

{==== 测试指标 ====}
{编号7: 测试函数，返回K线序号 0,1,2,3...}
{如果显示一条从左下到右上的斜线，说明DLL正常工作}
TEST:TDXDLL1(7, H, L, C);
```

### 调用格式说明

- `TDXDLL1` = 第1号DLL（由 dlls.ini 中 band1=chan.dll 决定）
- `TDXDLL2` = 第2号DLL（如果有 band2=xxx.dll）
- 第一个参数 = 函数编号（对应代码中 g_CalcFuncSets 的 nFuncMark）
- 后三个参数 = 传给函数的数据数组（通常是 H, L, C）

## 技术规范

### DLL接口（官方规范）

```cpp
// 函数签名
typedef void(*pPluginFUNC)(int DataLen, float* pfOUT, 
                           float* pfINa, float* pfINb, float* pfINc);

// 注册结构（必须1字节对齐）
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

## 版本历史

- v4.0.0 (2026-02-01): 修复接口规范
  - 修正 PluginTCalcFuncInfo 结构体（只需 nFuncMark + pCallFunc）
  - 修正 RegisterTdxFunc 签名为 `BOOL RegisterTdxFunc(PluginTCalcFuncInfo** pFun)`
  - 更新公式调用示例为标准 `TDXDLL1(编号, H, L, C)` 格式
- v1.3.0 (2025-01-25): P4阶段完成，准买卖点与类二买卖功能
- v1.2.0 (2025-01-22): P3阶段完成，123买卖点标准实现
- v1.1.0 (2025-01-18): P2阶段完成，递归引用体系
- v1.0.0 (2025-12-29): P1阶段完成，空壳DLL

## 许可证

MIT License
