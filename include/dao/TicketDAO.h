#pragma once

#include "ITicketDAO.h"
#include <mysql.h> // mysql文件
#include <stdio.h>
#include <iostream>
#include <string>

#define SQL_MAX 512		// sql语句字符数组最大值

//使用mysql数据库的DAO
class TicketDAO :public ITicketDAO
{
public:
    TicketDAO(MYSQL*);

    //工单相关
    bool createTicket(Ticket& ticket) override;
    bool approveTicket(const Ticket& ticket) override;
    bool dispatchTicket(const Ticket& ticket) override;
    bool completeTicket(const Ticket& ticket) override;



    std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string, std::string>& filter, int offset, int count) override;

    bool orderTransfer(const TicketExecutor& executor) override;
    unsigned long long getOrderCount() override;
    std::vector<std::string> getOrderClient() override;
    ~TicketDAO();
private:
    //mysql套件
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;

    //工具函数
    bool createConcreteTicket(Ticket& ticket);
    bool completeConcreteTicket(const Ticket& ticket);
    bool saveUploadFile(const TicketReproduce& ticket);
    void concreteTicketList(int work_order_id,std::shared_ptr<Ticket> vecElement,std::vector<std::shared_ptr<Ticket>>&);

};