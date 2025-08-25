#pragma once
#include <mysql.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include <mutex>
#include "util/ConnectionPool.h"
#include "util/DBConnectionManager.h"

/**
 * 基础DAO类，提供优化的数据库连接管理
 * 解决每个方法都获取连接的性能开销问题
 */
class BaseDAO {
protected:
    BaseDAO();
    virtual ~BaseDAO();

public:
    /**
     * 获取当前线程的数据库连接
     * 使用线程本地存储，避免重复获取连接的开销
     * @return MYSQL* 数据库连接指针
     */
    MYSQL* getConnection();

protected:

    /**
     * 释放当前线程的连接（可选调用）
     * 通常在长时间不使用时调用，连接会自动在线程结束时释放
     */
    void releaseConnection();

    /**
     * 检查连接是否有效，无效时重新获取
     * @return bool 连接是否有效
     */
    bool ensureConnection();

private:
    // 线程本地存储的连接管理
    static thread_local std::unique_ptr<ConnectionGuard> threadConnection_;
    static thread_local std::chrono::steady_clock::time_point lastUsed_;
    
    // 连接超时时间（秒），超过此时间未使用则释放连接
    static constexpr int CONNECTION_TIMEOUT = 300; // 5分钟
    
    /**
     * 检查连接是否超时
     * @return bool 是否超时
     */
    bool isConnectionTimeout();
};

/**
 * 事务管理器
 * 用于需要事务支持的操作，确保在同一个连接中执行多个SQL
 */
class TransactionManager {
public:
    explicit TransactionManager(BaseDAO* dao);
    ~TransactionManager();
    
    /**
     * 开始事务
     * @return bool 是否成功
     */
    bool begin();
    
    /**
     * 提交事务
     * @return bool 是否成功
     */
    bool commit();
    
    /**
     * 回滚事务
     * @return bool 是否成功
     */
    bool rollback();
    
    /**
     * 获取事务中的连接
     * @return MYSQL* 数据库连接指针
     */
    MYSQL* getConnection();
    
private:
    BaseDAO* dao_;
    MYSQL* connection_;
    bool inTransaction_;
    bool autoCommit_;
};