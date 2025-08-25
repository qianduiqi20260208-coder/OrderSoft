#include "dao/BaseDAO.h"
#include "util/Logger.h"
#include <chrono>

// 线程本地存储定义
thread_local std::unique_ptr<ConnectionGuard> BaseDAO::threadConnection_ = nullptr;
thread_local std::chrono::steady_clock::time_point BaseDAO::lastUsed_ = std::chrono::steady_clock::now();

BaseDAO::BaseDAO() {
    // 构造函数不需要特殊处理
}

BaseDAO::~BaseDAO() {
    // 析构函数不需要特殊处理，线程结束时会自动清理thread_local变量
}

MYSQL* BaseDAO::getConnection() {
    // 检查是否需要重新获取连接
    if (!threadConnection_ || !threadConnection_->isValid() || isConnectionTimeout()) {
        if (threadConnection_) {
            LOG_DEBUG("BaseDAO::getConnection 释放超时或无效的连接");
            threadConnection_.reset();
        }
        
        // 获取新连接
        auto conn = ConnectionPool::getInstance().getConnection();
        if (conn) {
            threadConnection_ = std::make_unique<ConnectionGuard>(conn);
            lastUsed_ = std::chrono::steady_clock::now();
            LOG_DEBUG("BaseDAO::getConnection 为线程获取新连接");
        } else {
            LOG_ERROR("BaseDAO::getConnection 获取数据库连接失败");
            return nullptr;
        }
    } else {
        // 更新最后使用时间
        lastUsed_ = std::chrono::steady_clock::now();
    }
    
    return threadConnection_ ? threadConnection_->get() : nullptr;
}

void BaseDAO::releaseConnection() {
    if (threadConnection_) {
        LOG_DEBUG("BaseDAO::releaseConnection 主动释放线程连接");
        threadConnection_.reset();
    }
}

bool BaseDAO::ensureConnection() {
    MYSQL* conn = getConnection();
    if (!conn) {
        return false;
    }
    
    // 检查连接是否真正可用
    if (mysql_ping(conn) != 0) {
        LOG_WARNING("BaseDAO::ensureConnection 连接ping失败，重新获取连接");
        threadConnection_.reset();
        conn = getConnection();
        return conn != nullptr;
    }
    
    return true;
}

bool BaseDAO::isConnectionTimeout() {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastUsed_);
    return duration.count() > CONNECTION_TIMEOUT;
}

// TransactionManager实现
TransactionManager::TransactionManager(BaseDAO* dao) 
    : dao_(dao), connection_(nullptr), inTransaction_(false), autoCommit_(true) {
    if (dao_) {
        connection_ = dao_->getConnection();
    }
}

TransactionManager::~TransactionManager() {
    if (inTransaction_) {
        LOG_WARNING("TransactionManager::~TransactionManager 事务未提交，自动回滚");
        rollback();
    }
    
    // 恢复自动提交模式
    if (connection_ && !autoCommit_) {
        mysql_autocommit(connection_, 1);
    }
}

bool TransactionManager::begin() {
    if (!connection_) {
        LOG_ERROR("TransactionManager::begin 数据库连接无效");
        return false;
    }
    
    if (inTransaction_) {
        LOG_WARNING("TransactionManager::begin 事务已经开始");
        return true;
    }
    
    // 关闭自动提交
    if (mysql_autocommit(connection_, 0) != 0) {
        LOG_ERROR("TransactionManager::begin 关闭自动提交失败: %s", mysql_error(connection_));
        return false;
    }
    
    autoCommit_ = false;
    inTransaction_ = true;
    LOG_DEBUG("TransactionManager::begin 事务开始");
    return true;
}

bool TransactionManager::commit() {
    if (!connection_ || !inTransaction_) {
        LOG_ERROR("TransactionManager::commit 无效的事务状态");
        return false;
    }
    
    if (mysql_commit(connection_) != 0) {
        LOG_ERROR("TransactionManager::commit 事务提交失败: %s", mysql_error(connection_));
        return false;
    }
    
    inTransaction_ = false;
    
    // 恢复自动提交模式
    mysql_autocommit(connection_, 1);
    autoCommit_ = true;
    
    LOG_DEBUG("TransactionManager::commit 事务提交成功");
    return true;
}

bool TransactionManager::rollback() {
    if (!connection_ || !inTransaction_) {
        LOG_ERROR("TransactionManager::rollback 无效的事务状态");
        return false;
    }
    
    if (mysql_rollback(connection_) != 0) {
        LOG_ERROR("TransactionManager::rollback 事务回滚失败: %s", mysql_error(connection_));
        return false;
    }
    
    inTransaction_ = false;
    
    // 恢复自动提交模式
    mysql_autocommit(connection_, 1);
    autoCommit_ = true;
    
    LOG_DEBUG("TransactionManager::rollback 事务回滚成功");
    return true;
}

MYSQL* TransactionManager::getConnection() {
    return connection_;
}