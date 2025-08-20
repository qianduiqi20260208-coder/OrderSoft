#pragma once
#include <mysql.h>

//封装成类便于维护，比如日后可能增加的多线程机制
class DBConnectionManager
{
public:
    static bool getConnection(MYSQL*& mysql);
    static bool ensureConnected(MYSQL* mysql);
    static void closeConnection(MYSQL* mysql);

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
