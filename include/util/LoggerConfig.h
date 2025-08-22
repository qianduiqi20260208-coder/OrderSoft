#ifndef LOGGER_CONFIG_H
#define LOGGER_CONFIG_H

#include "Logger.h"
#include "IniReader.h"
#include <string>

// 日志配置类
class LoggerConfig {
public:
    // 从配置文件初始化日志系统
    static void initFromConfig(const std::string& configPath = "config.ini");
    
    // 获取配置值
    static std::string getLogBasePath();
    static LogLevel getMinLogLevel();
    static int getLogRetentionDays();
    static bool getConsoleOutput();
    
private:
    static std::string s_logBasePath;
    static LogLevel s_minLogLevel;
    static int s_logRetentionDays;
    static bool s_consoleOutput;
    static bool s_initialized;
};

#endif // LOGGER_CONFIG_H