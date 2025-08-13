#pragma once
#include "IClientDAO.h"
#include <mysql.h>
#define SQL_MAX 512		// sql语句字符数组最大值


class ClientDAO :public IClientDAO
{
public:
    ClientDAO(MYSQL* ms);
    std::vector<User> getUser()override;
   
    ~ClientDAO();
private:
    //mysql套件
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
    // std::shared_ptr<Ticket> getUserConcreteOrder(std::string,int);
public:
    static bool downloadAttachment(std::shared_ptr<TicketReproduce>);
   
};