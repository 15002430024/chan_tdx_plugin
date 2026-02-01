#pragma once
// ============================================================================
// 缠论通达信DLL插件 - 通达信接口定义
// ============================================================================

#ifndef TDX_INTERFACE_H
#define TDX_INTERFACE_H

#include <windows.h>

// ============================================================================
// 通达信插件接口定义
// ============================================================================

// 计算函数类型定义
// nCount: K线数量
// pOut: 输出数组 (长度=nCount)
// pHigh: 最高价数组
// pLow: 最低价数组
// pClose: 收盘价数组
// pVol: 成交量数组
// pAmount: 成交额数组
// pParam: 用户参数数组
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

// 函数信息结构体
#pragma pack(push, 1)
struct PluginTCalcFuncInfo {
    WORD    nFuncMark;          // 函数标识 (固定为 0x0001)
    char    sName[32];          // 函数名称 (如 "CHAN_BI")
    BYTE    nParamCount;        // 参数个数 (0-8)
    BYTE    nParamType[8];      // 参数类型 (0=数值, 1=指标线)
    char    sParamName[8][32];  // 参数名称
    float   fParamMin[8];       // 参数最小值
    float   fParamMax[8];       // 参数最大值
    float   fParamDef[8];       // 参数默认值
    PluginTCalcFunc pCalcFunc;  // 计算函数指针
};
#pragma pack(pop)

// ============================================================================
// 导出函数声明
// ============================================================================

extern "C" {
    // 函数注册入口 - 通达信调用此函数获取插件信息
    // 参数: ppInfo - 返回函数信息数组指针
    //       pCount - 返回函数数量
    __declspec(dllexport) void __stdcall RegisterTdxFunc(PluginTCalcFuncInfo** ppInfo, int* pCount);
}

// ============================================================================
// 计算函数声明
// ============================================================================

// 分型标记函数
void __stdcall CHAN_FX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// 笔端点函数
void __stdcall CHAN_BI_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// 线段端点函数
void __stdcall CHAN_DUAN_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam);

// 中枢高点函数
void __stdcall CHAN_ZS_H_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam);

// 中枢低点函数
void __stdcall CHAN_ZS_L_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam);

// 买点信号函数
void __stdcall CHAN_BUY_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                             float* pClose, float* pVol, float* pAmount, float* pParam);

// 卖点信号函数
void __stdcall CHAN_SELL_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam);

// 背驰标记函数
void __stdcall CHAN_BC_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// ============================================================================
// 阶段二：递归引用系统函数声明
// ============================================================================

// 方向判断函数 (1=下跌趋势后, -1=上涨趋势后, 0=震荡)
void __stdcall CHAN_DIR_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                             float* pClose, float* pVol, float* pAmount, float* pParam);

// GG顶点价格函数 (参数: N=笔长度, IDX=序号1-5)
void __stdcall CHAN_GG_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// DD底点价格函数 (参数: N=笔长度, IDX=序号1-5)
void __stdcall CHAN_DD_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// HH顶点距离函数 (参数: N=笔长度, IDX=序号1-5)
void __stdcall CHAN_HH_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// LL底点距离函数 (参数: N=笔长度, IDX=序号1-5)
void __stdcall CHAN_LL_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam);

// 幅度检查函数 (参数: N=笔长度, TYPE: 1=KJA, 2=KJB, 3=二买幅度)
void __stdcall CHAN_AMP_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                             float* pClose, float* pVol, float* pAmount, float* pParam);

// ============================================================================
// 阶段四：综合信号函数声明
// ============================================================================

// 综合买点信号函数 (包含标准买点、类买点、准买点)
// 输出: 1=一买, 2=二买, 3=三买, 11=准一买, 12=准二买, 13=准三买, 21=类二买
void __stdcall CHAN_BUYX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam);

// 综合卖点信号函数 (与买点信号对称)
void __stdcall CHAN_SELLX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                               float* pClose, float* pVol, float* pAmount, float* pParam);

// ============================================================================
// 阶段五：扩展函数声明
// ============================================================================

// 中枢中轴函数 (输出ZZ = (ZG+ZD)/2)
void __stdcall CHAN_ZS_Z_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                              float* pClose, float* pVol, float* pAmount, float* pParam);

// 准买点信号函数 (11=准一买, 12=准二买, 13=准三买)
void __stdcall CHAN_PREBUY_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam);

// 准卖点信号函数 (-11=准一卖, -12=准二卖, -13=准三卖)
void __stdcall CHAN_PRESELL_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                 float* pClose, float* pVol, float* pAmount, float* pParam);

// 类二买信号函数 (21=类二买A, 22=类二买AAA)
void __stdcall CHAN_LIKE2B_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam);

// 类二卖信号函数 (-21=类二卖A, -22=类二卖AAA)
void __stdcall CHAN_LIKE2S_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam);

// 去包含后新K线标记函数 (1=新K线, 0=被合并)
void __stdcall CHAN_NEWBAR_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                                float* pClose, float* pVol, float* pAmount, float* pParam);

#endif // TDX_INTERFACE_H
