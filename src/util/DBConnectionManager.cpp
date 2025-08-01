#include "DBConnectionManager.h"
#include "IniReader.h"
#include <stdio.h>


bool DBConnectionManager::getConnection(MYSQL*& mysql)
{
    //从ini文件里读出数据库配置
    IniReader config;
    if (!config.load("../../config/config.ini")) { 
        printf("[error] function:getConnection 无法读取 config.ini 文件\n");
        return 0;
    }
    std::string dbHost = config.getString("database", "host");
    int dbPort = config.getInt("database", "port");
    std::string dbUser = config.getString("database", "user");
    std::string dbPass = config.getString("database", "password");
    std::string dbName = config.getString("database", "database");

	// 1.初始化数据库句柄
	mysql = mysql_init(NULL);
    if (!mysql) {
        printf("[error] function:getConnection mysql初始化失败！\n");
        return false;
    }

	// 2.设置字符编码
	//mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "gbk");

	// 3.连接数据库
	MYSQL *ret = mysql_real_connect(mysql, dbHost.c_str(), dbUser.c_str(), dbPass.c_str(), dbName.c_str(), dbPort, NULL, 0);
	if (ret == NULL) {
		printf("[error] function:getConnection 数据库连接失败！失败原因：%s\n", mysql_error(mysql));
		return false;
	}

    // *** 关键步骤：设置客户端连接的字符集为 utf8mb4 ***
    // 确保你的数据库、表、列都是 utf8mb4
    if (mysql_set_character_set(mysql, "utf8mb4")) {
        printf("[error] function:getConnection Error setting client character set to utf8mb4: %s\n", mysql_error(mysql));
    }

	// 选择数据库
	int res = mysql_select_db(mysql, dbName.c_str());
	if (res) {
		printf("[error] function:getConnection 选择数据库失败！失败原因%s\n", mysql_error(mysql));
		return false;
	}

    return true;
}

bool DBConnectionManager::ensureConnected(MYSQL *mysql)
{
    if(mysql_ping(mysql) !=0)
    {
        //释放资源并且重新连接
        closeConnection(mysql);
        return getConnection(mysql);
    }
    return true;
}

void DBConnectionManager::closeConnection(MYSQL *mysql)
{
    // 关闭数据库
	mysql_close(mysql);
}
