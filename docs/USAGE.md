# 使用手册

## 一、通达信公式基础

### 1.1 DLL调用语法

通达信调用外部DLL的语法格式：
```
变量名:="DLL文件名"$函数名(参数1,参数2,...);
```

本DLL文件名为`CHAN.DLL`，调用示例：
```
FX:="CHAN.DLL"$CHAN_FX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
```

### 1.2 参数说明

所有函数接收7个标准参数：

| 参数 | 说明 | 数据源 |
|------|------|--------|
| HIGH | 最高价数组 | 系统变量 |
| LOW | 最低价数组 | 系统变量 |
| OPEN | 开盘价数组 | 系统变量 |
| CLOSE | 收盘价数组 | 系统变量 |
| VOL | 成交量数组 | 系统变量 |
| AMOUNT | 成交额数组 | 系统变量 |
| PARAM | 附加参数 | 根据函数不同 |

---

## 二、核心指标公式

### 2.1 分型指标

```tdx
{=== 缠论分型指标 ===}
{文件名：缠论分型.tn6}

FX:="CHAN.DLL"$CHAN_FX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{画顶分型 - 红色向下箭头}
DRAWICON(FX=1, HIGH*1.01, 2);

{画底分型 - 绿色向上箭头}
DRAWICON(FX=-1, LOW*0.99, 1);
```

### 2.2 笔指标

```tdx
{=== 缠论笔指标 ===}
{文件名：缠论笔.tn6}

BI:="CHAN.DLL"$CHAN_BI(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{笔端点画圆点}
DRAWICON(BI>0, BI, 11);

{连接笔端点}
POLYLINE(BI>0, BI);
```

### 2.3 中枢指标

```tdx
{=== 缠论中枢指标 ===}
{文件名：缠论中枢.tn6}

ZS_H:="CHAN.DLL"$CHAN_ZS_H(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
ZS_L:="CHAN.DLL"$CHAN_ZS_L(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
ZS_Z:="CHAN.DLL"$CHAN_ZS_Z(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{画中枢区域}
DRAWBAND(ZS_H, RGB(255,200,200), ZS_L, RGB(200,255,200));

{画中枢中轴}
ZS_Z, COLORMAGENTA, LINETHICK2;
```

---

## 三、买卖点公式

### 3.1 标准买点

```tdx
{=== 缠论标准买点 ===}
{文件名：缠论买点.tn6}

BUY:="CHAN.DLL"$CHAN_BUY(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{一买 - 大红箭头}
DRAWICON(BUY=1, LOW*0.98, 1);
DRAWTEXT(BUY=1, LOW*0.96, '1买');

{二买 - 橙色箭头}
DRAWICON(BUY=2, LOW*0.98, 13);
DRAWTEXT(BUY=2, LOW*0.96, '2买');

{三买 - 黄色箭头}
DRAWICON(BUY=3, LOW*0.98, 14);
DRAWTEXT(BUY=3, LOW*0.96, '3买');
```

### 3.2 标准卖点

```tdx
{=== 缠论标准卖点 ===}
{文件名：缠论卖点.tn6}

SELL:="CHAN.DLL"$CHAN_SELL(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{一卖 - 大绿箭头}
DRAWICON(SELL=-1, HIGH*1.02, 2);
DRAWTEXT(SELL=-1, HIGH*1.04, '1卖');

{二卖}
DRAWICON(SELL=-2, HIGH*1.02, 12);
DRAWTEXT(SELL=-2, HIGH*1.04, '2卖');

{三卖}
DRAWICON(SELL=-3, HIGH*1.02, 15);
DRAWTEXT(SELL=-3, HIGH*1.04, '3卖');
```

### 3.3 综合买卖点（含准/类）

```tdx
{=== 缠论综合买卖点 ===}
{文件名：缠论综合信号.tn6}

BUYX:="CHAN.DLL"$CHAN_BUYX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
SELLX:="CHAN.DLL"$CHAN_SELLX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{===买点===}
{标准买点 1-3}
B1:=BUYX=1; B2:=BUYX=2; B3:=BUYX=3;
DRAWICON(B1, LOW*0.98, 1);
DRAWICON(B2, LOW*0.98, 13);
DRAWICON(B3, LOW*0.98, 14);

{准买点 11-13}
PB1:=BUYX=11; PB2:=BUYX=12; PB3:=BUYX=13;
DRAWICON(PB1, LOW*0.97, 5);
DRAWTEXT(PB1, LOW*0.95, '准1');
DRAWICON(PB2, LOW*0.97, 5);
DRAWTEXT(PB2, LOW*0.95, '准2');

{类二买 21-22}
L2B:=BUYX>=21 AND BUYX<=22;
DRAWICON(L2B, LOW*0.97, 6);
DRAWTEXT(L2B, LOW*0.95, '类2');

{===卖点===}
{标准卖点}
S1:=SELLX=-1; S2:=SELLX=-2; S3:=SELLX=-3;
DRAWICON(S1, HIGH*1.02, 2);
DRAWICON(S2, HIGH*1.02, 12);
DRAWICON(S3, HIGH*1.02, 15);

{准卖点}
PS1:=SELLX=-11;
DRAWICON(PS1, HIGH*1.03, 4);
DRAWTEXT(PS1, HIGH*1.05, '准1');
```

---

## 四、递归引用指标

### 4.1 方向判断

```tdx
{=== 缠论方向 ===}
DIR:="CHAN.DLL"$CHAN_DIR(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{方向着色}
STICKLINE(DIR=1, LOW, HIGH, 2, 0), COLORRED;    {看涨}
STICKLINE(DIR=-1, LOW, HIGH, 2, 0), COLORGREEN; {看跌}
STICKLINE(DIR=0, LOW, HIGH, 2, 0), COLORGRAY;   {震荡}
```

### 4.2 GG/DD序列

```tdx
{=== 缠论GG/DD序列 ===}
{附加参数n=1~5，表示第n个顶/底}

GG1:="CHAN.DLL"$CHAN_GG(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,1);
GG2:="CHAN.DLL"$CHAN_GG(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,2);
DD1:="CHAN.DLL"$CHAN_DD(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,1);
DD2:="CHAN.DLL"$CHAN_DD(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,2);

{画参考线}
GG1, COLORRED, LINETHICK1;
DD1, COLORGREEN, LINETHICK1;
```

---

## 五、选股公式

### 5.1 一买选股

```tdx
{=== 一买选股 ===}
{文件名：一买选股.tn6}

BUYX:="CHAN.DLL"$CHAN_BUYX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{今日出现一买信号}
XG:BUYX=1;
```

### 5.2 综合买点选股

```tdx
{=== 综合买点选股 ===}
BUYX:="CHAN.DLL"$CHAN_BUYX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{任意买点}
XG:BUYX>0;
```

### 5.3 强势二买选股

```tdx
{=== 强势二买选股 ===}
BUYX:="CHAN.DLL"$CHAN_BUYX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);

{类二买AAA（成功率90%）}
XG:BUYX=22;
```

---

## 六、配置说明

### 6.1 配置文件位置

`CZSC.ini`文件应与`chan.dll`放在同一目录。

### 6.2 主要配置项

```ini
[General]
MinBiLength = 5          ; 笔最小K线数
MinZSBiCount = 3         ; 中枢最小笔数
EnablePreSignal = 1      ; 启用准买卖点
EnableLikeSignal = 1     ; 启用类买卖点

[FirstBuy]
MAPeriod = 13            ; 一买均线周期
TimeWindow = 5           ; 一买时间窗口

[SecondBuy]
MAPeriod = 26            ; 二买均线周期
TimeWindow = 8           ; 二买时间窗口

[Performance]
EnableIncremental = 1    ; 启用增量计算
CacheSize = 100000       ; 缓存大小
```

---

## 七、使用技巧

### 7.1 多周期分析

在不同周期图表（日线、60分钟、30分钟）使用相同公式，可进行多周期共振分析。

### 7.2 信号过滤

结合成交量、MACD等指标过滤假信号：
```tdx
BUYX:="CHAN.DLL"$CHAN_BUYX(HIGH,LOW,OPEN,CLOSE,VOL,AMOUNT,0);
MACD_DIF:=EMA(CLOSE,12)-EMA(CLOSE,26);
MACD_DEA:=EMA(MACD_DIF,9);

{一买+MACD金叉确认}
XG:BUYX=1 AND CROSS(MACD_DIF, MACD_DEA);
```

### 7.3 性能优化

对于大量股票的批量计算，建议：
1. 启用增量计算 (`EnableIncremental=1`)
2. 使用适当的缓存大小
3. 避免在公式中重复调用相同函数

---

## 八、注意事项

1. **32位DLL**: 只能在32位通达信中使用
2. **数据量**: 建议加载足够的历史数据（至少200根K线）
3. **信号滞后**: 分型确认需要后续K线，会有1-2根K线的滞后
4. **风险提示**: 技术指标仅供参考，不构成投资建议
