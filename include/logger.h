#pragma once
// ============================================================================
// 缠论通达信DLL插件 - 日志模块
// ============================================================================

#ifndef LOGGER_H
#define LOGGER_H

// 必须在 windows.h 之前取消 ERROR 宏定义
#ifdef ERROR
#undef ERROR
#endif

#include <windows.h>

// 再次确保 ERROR 宏被取消
#ifdef ERROR
#undef ERROR
#endif

namespace chan {

// 日志级别
enum class LogLevel {
    LOG_OFF = 0,
    LOG_ERROR = 1,
    LOG_WARN = 2,
    LOG_INFO = 3,
    LOG_DEBUG = 4
};

// 初始化日志系统
void LogInit(LogLevel level = LogLevel::LOG_INFO);

// 设置日志级别
void LogSetLevel(LogLevel level);

// 日志输出函数
void LogError(const char* fmt, ...);
void LogWarn(const char* fmt, ...);
void LogInfo(const char* fmt, ...);
void LogDebug(const char* fmt, ...);

} // namespace chan

// ============================================================================
// 便捷宏定义
// ============================================================================

#ifdef _DEBUG
    #define CHAN_LOG_ERROR(fmt, ...) chan::LogError(fmt, ##__VA_ARGS__)
    #define CHAN_LOG_WARN(fmt, ...)  chan::LogWarn(fmt, ##__VA_ARGS__)
    #define CHAN_LOG_INFO(fmt, ...)  chan::LogInfo(fmt, ##__VA_ARGS__)
    #define CHAN_LOG_DEBUG(fmt, ...) chan::LogDebug(fmt, ##__VA_ARGS__)
#else
    #define CHAN_LOG_ERROR(fmt, ...) chan::LogError(fmt, ##__VA_ARGS__)
    #define CHAN_LOG_WARN(fmt, ...)  chan::LogWarn(fmt, ##__VA_ARGS__)
    #define CHAN_LOG_INFO(fmt, ...)  ((void)0)
    #define CHAN_LOG_DEBUG(fmt, ...) ((void)0)
#endif

#endif // LOGGER_H
