#include "util/ConnectionPool.h"
#include "util/Logger.h"
#include "util/IniReader.h"
#include <algorithm>

// Connection类实现
Connection::Connection() : mysql_(nullptr) {
    updateLastUsed();
}

Connection::~Connection() {
    disconnect();
}

bool Connection::connect(const std::string& host, int port, const std::string& user,
                        const std::string& password, const std::string& database) {
    // 初始化MySQL句柄
    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        LOG_ERROR("Connection::connect MySQL初始化失败");
        return false;
    }
    
    // 设置连接超时
    unsigned int timeout = 10;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(mysql_, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(mysql_, MYSQL_OPT_WRITE_TIMEOUT, &timeout);
    
    // 启用自动重连
    bool reconnect = true;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect);
    
    // 连接数据库
    MYSQL* ret = mysql_real_connect(mysql_, host.c_str(), user.c_str(), 
                                   password.c_str(), database.c_str(), 
                                   port, nullptr, 0);
    if (!ret) {
        LOG_ERROR("Connection::connect 数据库连接失败: %s", mysql_error(mysql_));
        mysql_close(mysql_);
        mysql_ = nullptr;
        return false;
    }
    
    // 设置字符集
    if (mysql_set_character_set(mysql_, "utf8mb4")) {
        LOG_ERROR("Connection::connect 设置字符集失败: %s", mysql_error(mysql_));
    }
    
    updateLastUsed();
    return true;
}

void Connection::disconnect() {
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
}

bool Connection::isConnected() {
    return mysql_ != nullptr;
}

bool Connection::ping() {
    if (!mysql_) {
        return false;
    }
    
    int result = mysql_ping(mysql_);
    if (result == 0) {
        updateLastUsed();
        return true;
    }
    
    LOG_WARNING("Connection::ping 连接检查失败: %s", mysql_error(mysql_));
    return false;
}

// ConnectionPool类实现
ConnectionPool& ConnectionPool::getInstance() {
    static ConnectionPool instance;
    return instance;
}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

bool ConnectionPool::initialize(const std::string& host, int port, const std::string& user,
                               const std::string& password, const std::string& database,
                               int minConnections, int maxConnections, int maxIdleTime) {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    if (initialized_) {
        LOG_WARNING("ConnectionPool::initialize 连接池已经初始化");
        return true;
    }
    
    host_ = host;
    port_ = port;
    user_ = user;
    password_ = password;
    database_ = database;
    minConnections_ = minConnections;
    maxConnections_ = maxConnections;
    maxIdleTime_ = maxIdleTime;
    
    // 创建最小连接数
    for (int i = 0; i < minConnections_; ++i) {
        auto conn = createConnection();
        if (conn) {
            idleConnections_.push(conn);
            totalConnections_++;
        } else {
            LOG_ERROR("ConnectionPool::initialize 创建初始连接失败");
            return false;
        }
    }
    
    // 启动清理线程
    shutdown_ = false;
    cleanupThread_ = std::thread(&ConnectionPool::cleanupThread, this);
    
    initialized_ = true;
    LOG_INFO("ConnectionPool::initialize 连接池初始化成功，初始连接数: %d", minConnections_);
    return true;
}

std::shared_ptr<Connection> ConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(poolMutex_);
    
    if (!initialized_) {
        LOG_ERROR("ConnectionPool::getConnection 连接池未初始化");
        return nullptr;
    }
    
    // 等待可用连接或创建新连接
    while (idleConnections_.empty() && totalConnections_ >= maxConnections_) {
        LOG_WARNING("ConnectionPool::getConnection 等待可用连接，当前总连接数: %d", totalConnections_.load());
        condition_.wait_for(lock, std::chrono::seconds(5));
        
        if (idleConnections_.empty() && totalConnections_ >= maxConnections_) {
            LOG_ERROR("ConnectionPool::getConnection 获取连接超时");
            return nullptr;
        }
    }
    LOG_INFO("ConnectionPool::getConnection 剩余连接数量: %d", idleConnections_.size());

    std::shared_ptr<Connection> conn;
    
    if (!idleConnections_.empty()) {
        // 从空闲连接池获取连接
        conn = idleConnections_.front();
        idleConnections_.pop();
        
        // 检查连接是否有效
        if (!conn->ping()) {
            LOG_WARNING("ConnectionPool::getConnection 连接已失效，重新创建");
            totalConnections_--;
            conn = createConnection();
            if (conn) {
                totalConnections_++;
            }
        }
    } else if (totalConnections_ < maxConnections_) {
        // 创建新连接
        conn = createConnection();
        if (conn) {
            totalConnections_++;
        }
    }
    
    if (conn) {
        activeConnections_++;
        conn->updateLastUsed();
        LOG_DEBUG("ConnectionPool::getConnection 获取连接成功，活跃连接数: %d", activeConnections_.load());
    } else {
        LOG_ERROR("ConnectionPool::getConnection 获取连接失败");
    }
    
    return conn;
}

void ConnectionPool::returnConnection(std::shared_ptr<Connection> conn) {
    if (!conn) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    if (shutdown_) {
        totalConnections_--;
        activeConnections_--;
        return;
    }
    
    // 检查连接是否有效
    if (conn->ping()) {
        idleConnections_.push(conn);
        LOG_DEBUG("ConnectionPool::returnConnection 连接归还成功，空闲连接数: %d", (int)idleConnections_.size());
    } else {
        LOG_WARNING("ConnectionPool::returnConnection 连接已失效，不归还到池中");
        totalConnections_--;
    }
    
    activeConnections_--;
    condition_.notify_one();
}

void ConnectionPool::shutdown() {
    std::lock_guard<std::mutex> lock(poolMutex_);
    
    if (shutdown_) {
        return;
    }
    
    shutdown_ = true;
    condition_.notify_all();
    
    // 等待清理线程结束
    if (cleanupThread_.joinable()) {
        cleanupThread_.join();
    }
    
    // 关闭所有连接
    while (!idleConnections_.empty()) {
        idleConnections_.pop();
    }
    
    totalConnections_ = 0;
    activeConnections_ = 0;
    initialized_ = false;
    
    LOG_INFO("ConnectionPool::shutdown 连接池已关闭");
}

std::shared_ptr<Connection> ConnectionPool::createConnection() {
    auto conn = std::make_shared<Connection>();
    if (conn->connect(host_, port_, user_, password_, database_)) {
        return conn;
    }
    return nullptr;
}

void ConnectionPool::cleanupThread() {
    LOG_INFO("ConnectionPool::cleanupThread 清理线程启动");
    
    while (!shutdown_) {
        std::this_thread::sleep_for(std::chrono::seconds(30)); // 每30秒检查一次
        
        if (shutdown_) {
            break;
        }
        
        std::lock_guard<std::mutex> lock(poolMutex_);
        
        auto now = std::chrono::steady_clock::now();
        std::queue<std::shared_ptr<Connection>> validConnections;
        int cleanedCount = 0;
        
        // 检查空闲连接是否超时
        while (!idleConnections_.empty()) {
            auto conn = idleConnections_.front();
            idleConnections_.pop();
            
            auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(now - conn->getLastUsed()).count();
            
            if (idleTime > maxIdleTime_ && totalConnections_ > minConnections_) {
                // 连接超时且总连接数大于最小连接数，清理该连接
                cleanedCount++;
                totalConnections_--;
            } else if (conn->ping()) {
                // 连接有效，保留
                validConnections.push(conn);
            } else {
                // 连接无效，清理
                cleanedCount++;
                totalConnections_--;
            }
        }
        
        // 将有效连接放回池中
        idleConnections_ = std::move(validConnections);
        
        if (cleanedCount > 0) {
            LOG_INFO("ConnectionPool::cleanupThread 清理了 %d 个连接，当前总连接数: %d", 
                    cleanedCount, totalConnections_.load());
        }
    }
    
    LOG_INFO("ConnectionPool::cleanupThread 清理线程结束");
}