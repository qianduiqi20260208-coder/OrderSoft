#pragma once
#include <mysql.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <chrono>

// 数据库连接包装类
class Connection {
public:
    Connection();
    ~Connection();
    
    bool connect(const std::string& host, int port, const std::string& user, 
                const std::string& password, const std::string& database);
    void disconnect();
    bool isConnected();
    bool ping();
    MYSQL* getMysql() { return mysql_; }
    
    // 更新最后使用时间
    void updateLastUsed() { lastUsed_ = std::chrono::steady_clock::now(); }
    std::chrono::steady_clock::time_point getLastUsed() const { return lastUsed_; }
    
private:
    MYSQL* mysql_;
    std::chrono::steady_clock::time_point lastUsed_;
};

// 连接池类
class ConnectionPool {
public:
    static ConnectionPool& getInstance();
    
    // 初始化连接池
    bool initialize(const std::string& host, int port, const std::string& user,
                   const std::string& password, const std::string& database,
                   int minConnections = 5, int maxConnections = 20,
                   int maxIdleTime = 300); // maxIdleTime in seconds
    
    // 获取连接
    std::shared_ptr<Connection> getConnection();
    
    // 归还连接
    void returnConnection(std::shared_ptr<Connection> conn);
    
    // 关闭连接池
    void shutdown();
    
    // 获取连接池状态
    int getActiveConnections() const { return activeConnections_; }
    int getIdleConnections() const { return idleConnections_.size(); }
    int getTotalConnections() const { return totalConnections_; }
    
private:
    ConnectionPool() = default;
    ~ConnectionPool();
    
    // 禁止拷贝和赋值
    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;
    
    // 创建新连接
    std::shared_ptr<Connection> createConnection();
    
    // 清理过期连接的后台线程
    void cleanupThread();
    
    // 连接参数
    std::string host_;
    int port_;
    std::string user_;
    std::string password_;
    std::string database_;
    
    // 连接池配置
    int minConnections_;
    int maxConnections_;
    int maxIdleTime_; // 最大空闲时间（秒）
    
    // 连接池状态
    std::queue<std::shared_ptr<Connection>> idleConnections_;
    std::atomic<int> activeConnections_{0};
    std::atomic<int> totalConnections_{0};
    
    // 线程同步
    mutable std::mutex poolMutex_;
    std::condition_variable condition_;
    
    // 后台清理线程
    std::thread cleanupThread_;
    std::atomic<bool> shutdown_{false};
    
    // 初始化标志
    std::atomic<bool> initialized_{false};
};

// RAII连接管理器
class ConnectionGuard {
public:
    explicit ConnectionGuard(std::shared_ptr<Connection> conn) : connection_(conn) {}
    
    ~ConnectionGuard() {
        if (connection_) {
            ConnectionPool::getInstance().returnConnection(connection_);
        }
    }
    
    MYSQL* get() const {
        return connection_ ? connection_->getMysql() : nullptr;
    }
    
    Connection* operator->() const {
        return connection_.get();
    }
    
    bool isValid() const {
        return connection_ && connection_->isConnected();
    }
    
    // 支持布尔检查
    operator bool() const {
        return isValid();
    }
    
    bool operator!() const {
        return !isValid();
    }
    
private:
    std::shared_ptr<Connection> connection_;
};