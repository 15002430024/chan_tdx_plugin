// ============================================================================
// 缠论通达信DLL插件 - 最小测试版本 v3
// 尝试与cl.dll相同的接口格式
// ============================================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstring>
#include <cstdio>

// ============================================================================
// 通达信插件接口定义 - 尝试不同的结构体布局
// ============================================================================

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

// 方式2: 尝试4字节对齐
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

// ============================================================================
// 调试日志
// ============================================================================

static void WriteDebugLog(const char* msg) {
    FILE* f = fopen("D:\\chan_debug.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, msg);
        fclose(f);
    }
}

// ============================================================================
// 测试计算函数
// ============================================================================

void __stdcall CHAN_FX_Calc(int nCount, float* pOut, float* pHigh, float* pLow, 
                            float* pClose, float* pVol, float* pAmount, float* pParam)
{
    WriteDebugLog("CHAN_FX_Calc 被调用!");
    
    if (pOut == NULL || nCount < 3) return;
    
    for (int i = 0; i < nCount; i++) {
        pOut[i] = 0.0f;
    }
    
    for (int i = 1; i < nCount - 1; i++) {
        if (pHigh[i] > pHigh[i-1] && pHigh[i] > pHigh[i+1]) {
            pOut[i] = 1.0f;  // 顶分型
        }
        else if (pLow[i] < pLow[i-1] && pLow[i] < pLow[i+1]) {
            pOut[i] = -1.0f; // 底分型
        }
    }
}

// ============================================================================
// 函数数组 - 结尾用空结构标记
// ============================================================================

static PluginTCalcFuncInfo g_FuncInfo[2];  // 1个函数 + 1个结束标记
static bool g_Initialized = false;

// ============================================================================
// 方式1: 返回函数数组指针（通达信可能用这种方式）
// ============================================================================

extern "C" __declspec(dllexport) PluginTCalcFuncInfo* __stdcall RegisterTdxFunc(short* pnFuncNum)
{
    char buf[256];
    sprintf(buf, "RegisterTdxFunc 被调用! sizeof(PluginTCalcFuncInfo)=%d", (int)sizeof(PluginTCalcFuncInfo));
    WriteDebugLog(buf);
    
    if (!g_Initialized) {
        memset(g_FuncInfo, 0, sizeof(g_FuncInfo));
        
        // 函数1: CHAN_FX - 尝试无参数版本
        g_FuncInfo[0].nFuncMark = 0x0001;
        strcpy(g_FuncInfo[0].sName, "CHAN_FX");
        g_FuncInfo[0].nParamCount = 0;  // 先试无参数
        g_FuncInfo[0].pCalcFunc = CHAN_FX_Calc;
        
        // 结束标记
        g_FuncInfo[1].nFuncMark = 0;
        
        g_Initialized = true;
        
        sprintf(buf, "函数已初始化: name=%s, paramCount=%d, calcFunc=%p", 
                g_FuncInfo[0].sName, g_FuncInfo[0].nParamCount, g_FuncInfo[0].pCalcFunc);
        WriteDebugLog(buf);
    }
    
    if (pnFuncNum != NULL) {
        *pnFuncNum = 1;
    }
    
    return g_FuncInfo;
}

// ============================================================================
// DLL入口点
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        WriteDebugLog("=== DLL_PROCESS_ATTACH ===");
        break;
    case DLL_PROCESS_DETACH:
        WriteDebugLog("=== DLL_PROCESS_DETACH ===");
        break;
    }
    return TRUE;
}
