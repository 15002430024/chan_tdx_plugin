# 通达信缠论DLL插件 (CZSC-DLL)

> 开源、可定制的通达信缠论技术分析DLL函数库

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Build](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-40%20passed-brightgreen)]()

---

## 📋 项目简介

本项目是一个开源的通达信缠论DLL插件，实现了完整的缠论技术分析功能，包括：

- ✅ K线去包含处理
- ✅ 分型识别（顶分型/底分型）
- ✅ 笔识别
- ✅ 中枢识别（ZG/ZD/ZZ）
- ✅ 递归引用系统（GG/DD/HH/LL序列）
- ✅ 标准买卖点（一买/二买/三买/一卖/二卖/三卖）
- ✅ 准买卖点（准一买/准二买/准三买等）
- ✅ 类买卖点（类二买A/AAA型）
- ✅ 综合信号整合

## 🚀 特性

- **高性能**: 100,000条K线仅需5ms完成分析
- **可配置**: 通过CZSC.ini文件自定义参数
- **完整测试**: 40+单元测试，覆盖率>80%
- **纯C++实现**: 无外部依赖，易于编译和部署
- **开源协议**: MIT协议，可自由使用和修改

## 📦 快速开始

### 1. 下载DLL

从[Releases](../../releases)页面下载最新版本的`chan.dll`。

### 2. 安装到通达信

将`chan.dll`和`CZSC.ini`复制到通达信安装目录的`T0001\`或`T0002\`文件夹。

### 3. 使用公式调用

```tdx
{缠论分型指标}
FX:="CHAN.DLL"$CHAN_FX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
DRAWICON(FX=1,HIGH,1);   {顶分型}
DRAWICON(FX=-1,LOW,2);   {底分型}
```

## 📚 文档

- [安装指南](INSTALL.md) - 详细的安装和编译说明
- [使用手册](USAGE.md) - 通达信公式示例
- [API文档](API.md) - DLL函数接口说明
- [更新日志](CHANGELOG.md) - 版本历史

## 🔧 编译

### 环境要求

- Windows 7/10/11
- Visual Studio 2019/2022
- CMake 3.15+

### 编译步骤

```bash
# 克隆仓库
git clone https://github.com/yourname/czsc-dll.git
cd czsc-dll

# 创建构建目录
mkdir build && cd build

# 配置（32位）
cmake -A Win32 ..

# 编译
cmake --build . --config Release

# 运行测试
.\bin\Release\test_chan_core.exe
```

## 📊 导出函数列表

| 函数名 | 功能 | 返回值说明 |
|--------|------|------------|
| CHAN_FX | 分型识别 | 1=顶, -1=底, 0=无 |
| CHAN_BI | 笔端点 | 笔的高/低点价格 |
| CHAN_ZS_H | 中枢高点 | ZG值 |
| CHAN_ZS_L | 中枢低点 | ZD值 |
| CHAN_ZS_Z | 中枢中轴 | (ZG+ZD)/2 |
| CHAN_BUY | 标准买点 | 1=一买, 2=二买, 3=三买 |
| CHAN_SELL | 标准卖点 | -1=一卖, -2=二卖, -3=三卖 |
| CHAN_BUYX | 综合买点 | 含准/类买点 |
| CHAN_SELLX | 综合卖点 | 含准/类卖点 |
| ... | | 共24个函数 |

完整列表请参考[API文档](API.md)。

## 🧪 测试结果

```
========================================
测试完成: 40/40 通过
========================================
性能测试: 100K K线 耗时 5ms
```

## 📁 项目结构

```
chan_tdx_plugin/
├── include/           # 头文件
│   ├── chan_core.h    # 核心算法声明
│   ├── chan_types.h   # 数据类型定义
│   ├── config_reader.h# 配置读取器
│   └── ...
├── src/               # 源代码
│   ├── chan_core.cpp  # 核心算法实现
│   ├── tdx_interface.cpp # 通达信接口
│   └── ...
├── test/              # 测试代码
├── docs/              # 文档
├── examples/          # 通达信公式示例
├── CZSC.ini           # 配置文件
├── CMakeLists.txt     # CMake配置
└── LICENSE            # MIT协议
```

## 🤝 贡献

欢迎提交Issue和Pull Request！

1. Fork本仓库
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 创建Pull Request

## 📜 协议

本项目采用 [MIT 协议](LICENSE) 开源。

## 🙏 致谢

- 缠中说禅博客原文
- 通达信软件
- 所有贡献者

---

**免责声明**: 本项目仅供学习研究使用，不构成投资建议。股市有风险，投资需谨慎。
