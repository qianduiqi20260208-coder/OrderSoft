#include "../../include/util/LoggerConfig.h"
#include <iostream>

// 静态成员变量定义
std::string LoggerConfig::s_logBasePath = "logs";
LogLevel LoggerConfig::s_minLogLevel = LogLevel::INFO_LEVEL;
int LoggerConfig::s_logRetentionDays = 30;
bool LoggerConfig::s_consoleOutput = true;
bool LoggerConfig::s_initialized = false;

// 从配置文件初始化日志系统
void LoggerConfig::initFromConfig(const std::string& configPath) {
    try {
        IniReader reader;
        if (!reader.load(configPath)) {
            throw std::runtime_error("Failed to load config file");
        }
        
        // 读取日志配置
        s_logBasePath = reader.getString("logging", "log_base_path", "logs");
        
        // 读取最小日志级别
        int levelInt = reader.getInt("logging", "min_log_level", 1);
        if (levelInt >= 0 && levelInt <= 4) {
            s_minLogLevel = static_cast<LogLevel>(levelInt);
        } else {
            s_minLogLevel = LogLevel::INFO_LEVEL;
            std::cerr << "Invalid log level in config, using INFO as default" << std::endl;
        }
        
        // 读取日志保留天数
        s_logRetentionDays = reader.getInt("logging", "log_retention_days", 30);
        
        // 读取控制台输出设置
        s_consoleOutput = reader.getBool("logging", "console_output", true);
        
        // 初始化日志系统
        bool success = Logger::getInstance().initialize(s_logBasePath, s_minLogLevel);
        if (success) {
            s_initialized = true;
            
            // 记录配置信息
            LOG_INFO("Logger configuration loaded:");
            LOG_INFO("  - Base path: %s", s_logBasePath.c_str());
            LOG_INFO("  - Min level: %s", Logger::getLevelString(s_minLogLevel).c_str());
            LOG_INFO("  - Retention days: %d", s_logRetentionDays);
            LOG_INFO("  - Console output: %s", s_consoleOutput ? "enabled" : "disabled");
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger from config: " << e.what() << std::endl;
        
        // 使用默认配置初始化
        bool success = Logger::getInstance().initialize(s_logBasePath, s_minLogLevel);
        if (success) {
            s_initialized = true;
            LOG_WARNING("Logger initialized with default configuration due to config error: %s", e.what());
        }
    }
}

// 获取配置值
std::string LoggerConfig::getLogBasePath() {
    return s_logBasePath;
}

LogLevel LoggerConfig::getMinLogLevel() {
    return s_minLogLevel;
}

int LoggerConfig::getLogRetentionDays() {
    return s_logRetentionDays;
}

bool LoggerConfig::getConsoleOutput() {
    return s_consoleOutput;
}