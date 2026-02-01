// ============================================================================
// 缠论通达信DLL插件 - DLL入口点
// ============================================================================

#include <windows.h>
#include "logger.h"

// DLL入口函数
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // DLL被加载时
        DisableThreadLibraryCalls(hModule);  // 禁用线程通知以提高性能
        
        #ifdef _DEBUG
        chan::LogInit(chan::LogLevel::LOG_DEBUG);
        #else
        chan::LogInit(chan::LogLevel::LOG_WARN);
        #endif
        
        CHAN_LOG_INFO("chan.dll 已加载 (版本: 1.0.0)");
        break;
        
    case DLL_PROCESS_DETACH:
        // DLL被卸载时
        CHAN_LOG_INFO("chan.dll 已卸载");
        break;
        
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        // 线程创建/销毁（已禁用）
        break;
    }
    
    return TRUE;
}
