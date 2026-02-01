# 安装指南

## 一、快速安装（使用预编译DLL）

### 1.1 下载DLL

从[Releases](../../releases)页面下载最新版本，包含：
- `chan.dll` - 主程序DLL
- `CZSC.ini` - 配置文件

### 1.2 安装到通达信

1. 找到通达信安装目录，例如：`C:\new_tdx\`
2. 将`chan.dll`复制到以下任一目录：
   - `C:\new_tdx\T0001\` （系统公式目录）
   - `C:\new_tdx\T0002\` （用户公式目录）
3. 将`CZSC.ini`复制到与DLL相同的目录

### 1.3 验证安装

1. 打开通达信软件
2. 按`Ctrl+F`打开公式管理器
3. 新建一个指标公式，输入：
   ```
   FX:="CHAN.DLL"$CHAN_FX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
   ```
4. 如果没有报错，说明安装成功

---

## 二、从源码编译

### 2.1 环境要求

| 组件 | 最低版本 | 推荐版本 |
|------|----------|----------|
| 操作系统 | Windows 7 SP1 | Windows 10/11 |
| Visual Studio | 2019 | 2022 |
| CMake | 3.15 | 3.25+ |

### 2.2 获取源码

```bash
git clone https://github.com/yourname/czsc-dll.git
cd czsc-dll
```

或下载ZIP包解压。

### 2.3 配置编译环境

**方法一：使用Visual Studio Developer Command Prompt**

1. 打开"Developer Command Prompt for VS 2022"
2. 进入项目目录

**方法二：使用普通命令行**

确保CMake在PATH中。

### 2.4 编译步骤

```bash
# 创建构建目录
mkdir build
cd build

# 配置项目（必须使用32位！）
cmake -A Win32 ..

# 编译Release版本
cmake --build . --config Release

# 编译Debug版本（可选）
cmake --build . --config Debug
```

### 2.5 编译输出

编译成功后，文件位于：
```
build/
├── bin/
│   └── Release/
│       ├── chan.dll        # 主DLL
│       └── test_chan_core.exe  # 测试程序
└── lib/
    └── Release/
        ├── chan.lib        # 导入库
        └── chan.exp        # 导出文件
```

### 2.6 运行测试

```bash
.\bin\Release\test_chan_core.exe
```

预期输出：
```
========================================
测试完成: 40/40 通过
========================================
```

---

## 三、常见问题

### 3.1 通达信提示"找不到DLL"

**原因**: DLL未放在正确目录，或缺少运行时库

**解决**:
1. 确保DLL在`T0001`或`T0002`目录
2. 如果使用Debug版本，需要安装VC运行时

### 3.2 编译时提示"x64编译器"

**原因**: 未使用32位编译配置

**解决**: 必须使用 `-A Win32` 参数
```bash
cmake -A Win32 ..
```

### 3.3 公式调用报错

**原因**: 函数名拼写错误或参数数量不对

**解决**: 检查函数名是否正确（区分大小写）

### 3.4 配置文件不生效

**原因**: CZSC.ini文件位置不正确

**解决**: 确保CZSC.ini与chan.dll在同一目录

---

## 四、目录结构说明

```
安装后的通达信目录：
C:\new_tdx\
├── T0001/
│   ├── chan.dll        # ← DLL放这里
│   └── CZSC.ini        # ← 配置文件放这里
├── T0002/
│   └── ...
└── TdxW.exe
```

---

## 五、卸载

1. 删除`T0001\chan.dll`
2. 删除`T0001\CZSC.ini`
3. 删除相关的自定义公式

---

## 六、联系支持

如遇到问题，请：
1. 查看[常见问题](FAQ.md)
2. 提交[Issue](../../issues)
3. 查看[讨论区](../../discussions)
