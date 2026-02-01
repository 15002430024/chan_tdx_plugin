# 缠论通达信 DLL 编译指南

## 编译要求

**重要**: 通达信仅支持 **32位 DLL**，必须使用 32 位编译环境。

## 方案一：Visual Studio（推荐）

### 1. 安装 Visual Studio

下载 [Visual Studio 2022 Community](https://visualstudio.microsoft.com/zh-hans/vs/community/)（免费）

安装时选择以下组件：
- ✅ 使用 C++ 的桌面开发
- ✅ MSVC v143 - VS 2022 C++ x64/x86 生成工具

### 2. 编译步骤

```batch
# 打开 "x86 Native Tools Command Prompt for VS 2022"
cd 到项目目录

# 使用 MSBuild 编译
msbuild chan.sln /p:Configuration=Release /p:Platform=Win32

# 或者直接双击 chan.sln 用 Visual Studio 打开编译
```

### 3. 输出

编译成功后，DLL 位于：`bin\Release\chan.dll`

---

## 方案二：MinGW-w64 32位

### 1. 下载 32 位 MinGW

从 [WinLibs](https://winlibs.com/) 下载 **i686** (32位) 版本：
- 选择 `i686-posix-dwarf` 版本

或从 [MSYS2](https://www.msys2.org/) 安装：
```bash
pacman -S mingw-w64-i686-gcc
```

### 2. 编译命令

```batch
set PATH=C:\mingw32\bin;%PATH%
cd 到项目目录

g++ -shared -static -std=c++17 -O2 -Wall ^
    -DNDEBUG -DWIN32_LEAN_AND_MEAN -DNOMINMAX ^
    -I"include" ^
    -o "bin/chan.dll" ^
    "src/dllmain.cpp" ^
    "src/tdx_interface.cpp" ^
    "src/logger.cpp" ^
    -lkernel32 -luser32
```

---

## 方案三：使用 GitHub Actions 云编译

创建 `.github/workflows/build.yml`：

```yaml
name: Build DLL

on: [push, workflow_dispatch]

jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
      
      - uses: microsoft/setup-msbuild@v1
      
      - name: Build Release
        run: msbuild chan.sln /p:Configuration=Release /p:Platform=Win32
        working-directory: chan_tdx_plugin
        
      - uses: actions/upload-artifact@v4
        with:
          name: chan-dll
          path: chan_tdx_plugin/bin/Release/chan.dll
```

然后将代码推送到 GitHub，Actions 会自动编译并提供下载。

---

## 验证 DLL

编译完成后，验证 DLL 是否正确：

### 1. 检查架构（必须是 32 位）

```powershell
# 使用 dumpbin (需要 VS)
dumpbin /headers bin/chan.dll | findstr "machine"
# 应输出: 14C machine (x86)

# 或使用 file 命令 (Git Bash)
file bin/chan.dll
# 应输出: PE32 executable (DLL) ...
```

### 2. 检查导出函数

```powershell
# 使用 dumpbin
dumpbin /exports bin/chan.dll

# 或使用 objdump
objdump -p bin/chan.dll | findstr "RegisterTdxFunc"
```

应看到 `RegisterTdxFunc` 导出。

---

## 安装到通达信

1. **复制 DLL**
   ```
   将 chan.dll 复制到: 通达信安装目录\T0002\dlls\
   ```

2. **注册 DLL**（编辑 `T0002\dlls\dlls.ini`）
   ```ini
   [BAND]
   band1=chan.dll
   ```

3. **重启通达信**

4. **测试调用**

   在公式编辑器中创建测试公式：
   ```
   TEST:CHAN_FX(4);
   ```
   
   如果能正常加载（输出全 0，无报错），则 P1 验收通过。

---

## 常见问题

### Q: 提示"找不到入口点"
A: 检查 DLL 是否正确导出 `RegisterTdxFunc`，使用 `dumpbin /exports` 查看。

### Q: 通达信崩溃
A: 1) 确保是 32 位 DLL；2) 检查调用约定是否为 `__stdcall`

### Q: 无法加载 DLL
A: 检查 `dlls.ini` 配置是否正确，DLL 文件名是否一致。

---

## 项目文件说明

```
chan_tdx_plugin/
├── chan.sln              # VS 解决方案文件
├── chan.vcxproj          # VS 项目文件  
├── chan.def              # DLL 导出定义
├── CMakeLists.txt        # CMake 构建文件
├── build.bat             # VS 一键编译脚本
├── build_mingw.bat       # MinGW 编译脚本
├── include/
│   ├── tdx_interface.h   # 通达信接口定义
│   ├── chan_types.h      # 核心数据类型
│   └── logger.h          # 日志模块
└── src/
    ├── dllmain.cpp       # DLL 入口
    ├── tdx_interface.cpp # 接口实现
    └── logger.cpp        # 日志实现
```
