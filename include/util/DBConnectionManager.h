#pragma once
#include <mysql.h>
#include <memory>
#include "util/ConnectionPool.h"

//封装成类便于维护，比如日后可能增加的多线程机制
class DBConnectionManager
{
public:
    // 初始化连接池
    static bool initializePool();
    
    // 获取连接池连接（推荐使用）
    static ConnectionGuard getPoolConnection();
    
    // 兼容旧接口（已废弃，建议使用连接池）
    static bool getConnection(MYSQL*& mysql);
    static bool ensureConnected(MYSQL* mysql);
    static void closeConnection(MYSQL* mysql);
    
    // 关闭连接池
    static void shutdownPool();

};

//操作日志模块的资源管理需求
class DBConnectionManagerForOperationLog
{
public:
    MYSQL* mysql;
    DBConnectionManagerForOperationLog(){DBConnectionManager::getConnection(mysql);}
    bool ensureConnected(){ return DBConnectionManager::ensureConnected(mysql);}
    ~DBConnectionManagerForOperationLog(){DBConnectionManager::closeConnection(mysql);}

};
