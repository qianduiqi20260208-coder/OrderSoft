#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <cstdarg>

// 日志级别枚举
enum class LogLevel {
    DEBUG_LEVEL = 0,
    INFO_LEVEL = 1,
    WARNING_LEVEL = 2,
    ERROR_LEVEL = 3,
    FATAL_LEVEL = 4
};

// 日志管理器类
class Logger {
public:
    // 获取单例实例
    static Logger& getInstance();
    
    // 初始化日志系统
    bool initialize(const std::string& baseLogPath = "logs", LogLevel minLevel = LogLevel::INFO_LEVEL);
    // 写入日志
    void log(LogLevel level, const char* file, int line, const char* format, ...);
    
    // 设置最小日志级别
    void setMinLogLevel(LogLevel level);
    
    // 获取日志级别字符串
    static std::string getLevelString(LogLevel level);
    
    // 清理过期日志文件
    void cleanOldLogs(int daysToKeep = 30);
    
    // 析构函数
    ~Logger();
    
private:
    Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    // 获取当前日期字符串 (YYYY-MM-DD)
    std::string getCurrentDate();
    
    // 获取当前时间字符串 (HH:MM:SS)
    std::string getCurrentTime();
    
    // 获取日志文件路径
    std::string getLogFilePath(LogLevel level, const std::string& date);
    
    // 确保目录存在
    void ensureDirectoryExists(const std::string& path);
    
    // 写入到文件
    void writeToFile(LogLevel level, const std::string& message);
    
private:
    std::string m_baseLogPath = "logs";  // 基础日志路径
    LogLevel m_minLogLevel = LogLevel::INFO_LEVEL;  // 最小日志级别
    std::mutex m_mutex;  // 线程安全锁
    bool m_initialized = false;  // 是否已初始化
};

// 便捷的日志宏定义
#define LOG_DEBUG(format, ...) \
    Logger::getInstance().log(LogLevel::DEBUG_LEVEL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) \
    Logger::getInstance().log(LogLevel::INFO_LEVEL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_WARNING(format, ...) \
    Logger::getInstance().log(LogLevel::WARNING_LEVEL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    Logger::getInstance().log(LogLevel::ERROR_LEVEL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#define LOG_FATAL(format, ...) \
    Logger::getInstance().log(LogLevel::FATAL_LEVEL, __FILE__, __LINE__, format, ##__VA_ARGS__)

// 兼容现有代码的宏定义
#define LOGGER_PRINTF(format, ...) \
    Logger::getInstance().log(LogLevel::ERROR_LEVEL, __FILE__, __LINE__, format, ##__VA_ARGS__)

#endif // LOGGER_H