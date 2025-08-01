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