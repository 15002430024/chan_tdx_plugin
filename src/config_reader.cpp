// ============================================================================
// 缠论通达信DLL插件 - INI配置文件读取实现
// ============================================================================

#include "config_reader.h"
#include "chan_core.h"
#include "logger.h"
#include <sstream>
#include <fstream>

namespace chan {

// ============================================================================
// ConfigReader 实现
// ============================================================================

ConfigReader::ConfigReader() {}

ConfigReader::~ConfigReader() {}

std::string ConfigReader::GetDllDirectory() {
    char path[MAX_PATH] = {0};
    HMODULE hModule = NULL;
    
    // 获取当前DLL的模块句柄
    if (GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&GetDllDirectory,
            &hModule)) {
        GetModuleFileNameA(hModule, path, MAX_PATH);
        
        // 去掉文件名，保留目录
        std::string full_path(path);
        size_t pos = full_path.find_last_of("\\/");
        if (pos != std::string::npos) {
            return full_path.substr(0, pos + 1);
        }
    }
    
    return ".\\";
}

bool ConfigReader::LoadConfig(const std::string& ini_path) {
    // 确定配置文件路径
    if (ini_path.empty()) {
        m_ini_path = GetDllDirectory() + "CZSC.ini";
    } else {
        m_ini_path = ini_path;
    }
    
    CHAN_LOG_INFO("加载配置文件: %s", m_ini_path.c_str());
    
    // 检查文件是否存在
    std::ifstream file(m_ini_path);
    if (!file.good()) {
        CHAN_LOG_WARN("配置文件不存在，使用默认配置: %s", m_ini_path.c_str());
        m_loaded = false;
        return false;
    }
    file.close();
    
    // 读取 [General] 节
    m_config.min_bi_length = ReadInt("General", "MinBiLength", 5);
    m_config.min_zs_bi_count = ReadInt("General", "MinZSBiCount", 3);
    m_config.enable_pre_signal = ReadBool("General", "EnablePreSignal", true);
    m_config.enable_like_signal = ReadBool("General", "EnableLikeSignal", true);
    m_config.strict_bi = ReadBool("General", "StrictBi", true);
    
    // 读取 [FirstBuy] 节
    m_config.first_buy_ma_period = ReadInt("FirstBuy", "MAPeriod", 13);
    m_config.first_buy_time_window = ReadInt("FirstBuy", "TimeWindow", 5);
    
    // 读取 [SecondBuy] 节
    m_config.second_buy_ma_period = ReadInt("SecondBuy", "MAPeriod", 26);
    m_config.second_buy_time_window = ReadInt("SecondBuy", "TimeWindow", 8);
    
    // 读取 [ThirdBuy] 节
    m_config.third_buy_ma_period = ReadInt("ThirdBuy", "MAPeriod", 13);
    m_config.third_buy_time_window = ReadInt("ThirdBuy", "TimeWindow", 5);
    
    // 读取 [PreBuy] 节
    m_config.pre_first_time_window = ReadInt("PreBuy", "FirstTimeWindow", 8);
    m_config.pre_second_time_window = ReadInt("PreBuy", "SecondTimeWindow", 10);
    
    // 读取 [Performance] 节
    m_config.enable_incremental = ReadBool("Performance", "EnableIncremental", true);
    m_config.cache_size = ReadInt("Performance", "CacheSize", 100000);
    
    m_loaded = true;
    
    CHAN_LOG_INFO("配置加载成功: MinBiLength=%d, EnablePreSignal=%d, EnableIncremental=%d",
                  m_config.min_bi_length, m_config.enable_pre_signal, m_config.enable_incremental);
    
    return true;
}

int ConfigReader::ReadInt(const char* section, const char* key, int default_val) {
    return GetPrivateProfileIntA(section, key, default_val, m_ini_path.c_str());
}

bool ConfigReader::ReadBool(const char* section, const char* key, bool default_val) {
    int val = GetPrivateProfileIntA(section, key, default_val ? 1 : 0, m_ini_path.c_str());
    return val != 0;
}

float ConfigReader::ReadFloat(const char* section, const char* key, float default_val) {
    char buffer[64] = {0};
    GetPrivateProfileStringA(section, key, "", buffer, sizeof(buffer), m_ini_path.c_str());
    
    if (buffer[0] == '\0') {
        return default_val;
    }
    
    return static_cast<float>(atof(buffer));
}

ChanConfig ConfigReader::ToChanConfig() const {
    ChanConfig config;
    config.min_bi_len = m_config.min_bi_length;
    config.min_zs_bi_count = m_config.min_zs_bi_count;
    config.strict_bi = m_config.strict_bi;
    config.enable_pre_signals = m_config.enable_pre_signal;
    config.enable_like_signals = m_config.enable_like_signal;
    return config;
}

// ============================================================================
// 全局配置访问
// ============================================================================

static ConfigReader g_ConfigReader;

ConfigReader& GetGlobalConfigReader() {
    return g_ConfigReader;
}

bool InitGlobalConfig() {
    return g_ConfigReader.LoadConfig();
}

} // namespace chan
