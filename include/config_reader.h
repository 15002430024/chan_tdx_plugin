#pragma once
// ============================================================================
// 缠论通达信DLL插件 - INI配置文件读取模块
// ============================================================================

#ifndef CONFIG_READER_H
#define CONFIG_READER_H

#include <string>
#include <windows.h>

namespace chan {

// 前向声明
struct ChanConfig;

// ============================================================================
// 配置结构体 (扩展版)
// ============================================================================

struct ChanIniConfig {
    // [General] 通用设置
    int min_bi_length = 5;          // 笔最小K线数
    int min_zs_bi_count = 3;        // 中枢最小笔数
    bool enable_pre_signal = true;  // 启用准买卖点
    bool enable_like_signal = true; // 启用类买卖点
    bool strict_bi = true;          // 严格笔定义
    
    // [FirstBuy] 一买参数
    int first_buy_ma_period = 13;   // 均线周期
    int first_buy_time_window = 5;  // 时间窗口
    
    // [SecondBuy] 二买参数
    int second_buy_ma_period = 26;  // 均线周期
    int second_buy_time_window = 8; // 时间窗口
    
    // [ThirdBuy] 三买参数
    int third_buy_ma_period = 13;   // 均线周期
    int third_buy_time_window = 5;  // 时间窗口
    
    // [PreBuy] 准买点参数
    int pre_first_time_window = 8;   // 准一买时间窗口
    int pre_second_time_window = 10; // 准二买时间窗口
    
    // [Performance] 性能参数
    bool enable_incremental = true; // 启用增量计算
    int cache_size = 100000;        // 缓存大小
};

// ============================================================================
// 配置读取器类
// ============================================================================

class ConfigReader {
public:
    ConfigReader();
    ~ConfigReader();
    
    // 加载配置文件 (从DLL同目录加载CZSC.ini)
    bool LoadConfig(const std::string& ini_path = "");
    
    // 获取配置
    const ChanIniConfig& GetConfig() const { return m_config; }
    
    // 应用配置到ChanConfig (实现在.cpp文件中)
    ChanConfig ToChanConfig() const;
    
    // 获取配置文件路径
    std::string GetIniPath() const { return m_ini_path; }
    
    // 检查配置是否已加载
    bool IsLoaded() const { return m_loaded; }

private:
    // 读取整数值
    int ReadInt(const char* section, const char* key, int default_val);
    
    // 读取布尔值
    bool ReadBool(const char* section, const char* key, bool default_val);
    
    // 读取浮点值
    float ReadFloat(const char* section, const char* key, float default_val);
    
    // 获取DLL所在目录
    static std::string GetDllDirectory();
    
    ChanIniConfig m_config;
    std::string m_ini_path;
    bool m_loaded = false;
};

// ============================================================================
// 全局配置访问
// ============================================================================

// 获取全局配置读取器实例
ConfigReader& GetGlobalConfigReader();

// 初始化全局配置 (DLL加载时调用)
bool InitGlobalConfig();

} // namespace chan

#endif // CONFIG_READER_H
