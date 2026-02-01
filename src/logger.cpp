// ============================================================================
// 缠论通达信DLL插件 - 日志模块实现
// ============================================================================

#include "logger.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

namespace chan {

// 全局日志级别
static LogLevel g_LogLevel = LogLevel::LOG_INFO;

// 日志前缀
static const char* g_LogPrefix[] = {
    "",         // OFF
    "[ERROR]",  // ERROR
    "[WARN] ",  // WARN
    "[INFO] ",  // INFO
    "[DEBUG]"   // DEBUG
};

// ============================================================================
// 内部函数
// ============================================================================

static void LogOutput(LogLevel level, const char* fmt, va_list args)
{
    if (level > g_LogLevel || level == LogLevel::LOG_OFF) {
        return;
    }
    
    char buffer[1024];
    char message[1200];
    
    // 格式化用户消息
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    // 获取当前时间
    time_t now = time(nullptr);
    struct tm* tm_now = localtime(&now);
    
    // 构造完整日志消息
    snprintf(message, sizeof(message), 
             "[CHAN %02d:%02d:%02d] %s %s\n",
             tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,
             g_LogPrefix[static_cast<int>(level)],
             buffer);
    
    // 输出到调试器
    OutputDebugStringA(message);
    
    // 如果是错误级别，也输出到stderr
    if (level == LogLevel::LOG_ERROR) {
        fprintf(stderr, "%s", message);
    }
}

// ============================================================================
// 公共接口
// ============================================================================

void LogInit(LogLevel level)
{
    g_LogLevel = level;
    LogInfo("日志系统初始化, 级别=%d", static_cast<int>(level));
}

void LogSetLevel(LogLevel level)
{
    g_LogLevel = level;
}

void LogError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogOutput(LogLevel::LOG_ERROR, fmt, args);
    va_end(args);
}

void LogWarn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogOutput(LogLevel::LOG_WARN, fmt, args);
    va_end(args);
}

void LogInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogOutput(LogLevel::LOG_INFO, fmt, args);
    va_end(args);
}

void LogDebug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    LogOutput(LogLevel::LOG_DEBUG, fmt, args);
    va_end(args);
}

} // namespace chan
