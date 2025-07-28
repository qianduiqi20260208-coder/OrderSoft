#pragma once
#include "IUserDAO.h"
#include <mysql.h>
#define SQL_MAX 512		// sql语句字符数组最大值


class UserDAO :public IUserDAO
{
public:
    UserDAO(MYSQL* ms);
    std::vector<User> getUser()override;
    //返回一个空指针放在vector里代表查询失败
    std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber)override;
    ~UserDAO();
private:
    //mysql套件
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
private:
    bool downloadAttachment(std::shared_ptr<TicketReproduce>);
};