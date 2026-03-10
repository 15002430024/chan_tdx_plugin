# 回归基线数据目录

> 用于 v7.5 架构收敛前后的回归比对

## 文件命名约定

| 文件名 | 内容 |
|--------|------|
| `baseline_999999_daily.csv` | 上证指数 日线 基线数据 |
| `baseline_600519_5min.csv` | 贵州茅台 5分钟 基线数据 |

## CSV 列结构

```
bar_index,BI,ZSZG,ZSZD,BSIG,SSIG,KXG,KXD
```

| 列名 | 对应DLL编号 | 含义 |
|------|-------------|------|
| BI | TDXDLL1(2) | 1=顶端点, -1=底端点 |
| ZSZG | TDXDLL1(3) | 中枢高点 |
| ZSZD | TDXDLL1(4) | 中枢低点 |
| BSIG | TDXDLL1(7) | 买点信号 |
| SSIG | TDXDLL1(8) | 卖点信号 |
| KXG | TDXDLL1(20) | 顶端点价格 |
| KXD | TDXDLL1(21) | 底端点价格 |

## 采集方式

当前 DLL 中已植入临时基线导出代码（`DumpBaselineCSV`）。

### 操作步骤

1. 编译 `chan.dll`（Release 模式）
2. 将编译产物复制到 `T0002/dlls/chan.dll`
3. 启动通达信
4. 打开 **999999 上证指数 日线** → DLL 会自动将基线写入 `D:\chan_baseline_<N>.csv`
5. **重启通达信**（重置 `g_BaselineDumped` 标志位）
6. 打开 **600519 贵州茅台 5分钟** → 产生另一个 CSV 文件
7. 将两个 CSV 文件拷贝到本目录并重命名

```powershell
copy D:\chan_baseline_*.csv test\baseline\
# 根据 bar 数量辨别文件并重命名
ren baseline_<日线bar数>.csv baseline_999999_daily.csv
ren baseline_<5分钟bar数>.csv baseline_600519_5min.csv
```

## 注意

- 基线代码中 `g_BaselineDumped` 是 static bool，每次 DLL 加载仅导出一次
- 如需采集第二组数据，需重启通达信（DLL 重新加载）
- 基线采集完成后，Phase 1 开始时会删除临时导出代码
