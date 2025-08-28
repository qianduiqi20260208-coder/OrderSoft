#include "../../include/util/Logger.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <algorithm>


// 获取单例实例
Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

// 初始化日志系统
bool Logger::initialize(const std::string& baseLogPath, LogLevel minLevel) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_baseLogPath = baseLogPath;
    m_minLogLevel = minLevel;
    
    
    // 确保基础日志目录存在
    try {
        ensureDirectoryExists(m_baseLogPath);
        m_initialized = true;
        
        // 记录初始化日志
        log(LogLevel::INFO_LEVEL, __FILE__, __LINE__, "Logger initialized with base path: %s, min level: %s", 
            m_baseLogPath.c_str(), getLevelString(m_minLogLevel).c_str());
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        return false;
    }
}

// 写入日志
void Logger::log(LogLevel level, const char* file, int line, const char* format, ...) {
    // 检查日志级别
    if (level < m_minLogLevel) {
        return;
    }
    
    // 如果未初始化，使用默认设置初始化
    if (!m_initialized) {
        initialize();
    }
    
    // 格式化消息
    va_list args;
    va_start(args, format);
    
    char buffer[4096];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 构建完整的日志消息
    std::ostringstream oss;
    oss << "[" << getCurrentTime() << "] "
        << "[" << getLevelString(level) << "] "
        << "[" << std::filesystem::path(file).filename().string() << ":" << line << "] "
        << buffer;
    
    std::string message = oss.str();
    
    // 同时输出到控制台和文件
    std::cout << message << std::endl;
    writeToFile(level, message);
}

// 设置最小日志级别
void Logger::setMinLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_minLogLevel = level;
}

// 获取日志级别字符串
std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG_LEVEL:   return "DEBUG";
        case LogLevel::INFO_LEVEL:    return "INFO";
        case LogLevel::WARNING_LEVEL: return "WARNING";
        case LogLevel::ERROR_LEVEL:   return "ERROR";
        case LogLevel::FATAL_LEVEL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

// 清理过期日志文件
void Logger::cleanOldLogs(int daysToKeep) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        auto now = std::chrono::system_clock::now();
        auto cutoffTime = now - std::chrono::hours(24 * daysToKeep);
        
        // 遍历日志目录
        if (std::filesystem::exists(m_baseLogPath)) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(m_baseLogPath)) {
                if (entry.is_regular_file()) {
                    auto fileTime = std::filesystem::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
                    
                    if (sctp < cutoffTime) {
                        std::filesystem::remove(entry.path());
                        log(LogLevel::INFO_LEVEL, __FILE__, __LINE__, "Removed old log file: %s",
                            entry.path().string().c_str());
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        log(LogLevel::ERROR_LEVEL, __FILE__, __LINE__, "Failed to clean old logs: %s", e.what());
    }
}

// 析构函数
Logger::~Logger() {
    if (m_initialized) {
        log(LogLevel::INFO_LEVEL, __FILE__, __LINE__, "Logger shutting down");
    }
}

// 获取当前日期字符串 (YYYY-MM-DD)
std::string Logger::getCurrentDate() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

// 获取当前时间字符串 (HH:MM:SS)
std::string Logger::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // 获取毫秒
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

// 获取日志文件路径
std::string Logger::getLogFilePath(LogLevel level, const std::string& date) {
    std::string levelDir = getLevelString(level);
    std::transform(levelDir.begin(), levelDir.end(), levelDir.begin(), ::tolower);
    
    std::filesystem::path logPath = std::filesystem::path(m_baseLogPath) / date / levelDir;
    std::string fileName = levelDir + "_" + date + ".log";
    
    return (logPath / fileName).string();
}

// 确保目录存在
void Logger::ensureDirectoryExists(const std::string& path) {
    std::filesystem::path dirPath(path);
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directories(dirPath);
    }
}

// 写入到文件
void Logger::writeToFile(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    try {
        std::string date = getCurrentDate();
        std::string filePath = getLogFilePath(level, date);
        
        // 确保目录存在
        std::filesystem::path fileDir = std::filesystem::path(filePath).parent_path();
        ensureDirectoryExists(fileDir.string());
        
        // 写入文件
        std::ofstream file(filePath, std::ios::app);
        if (file.is_open()) {
            file << message << std::endl;
            file.close();
        } else {
            std::cerr << "Failed to open log file: " << filePath << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to write to log file: " << e.what() << std::endl;
    }
}