#pragma once

#include "ITicketDAO.h"
#include "BaseDAO.h"
#include <mysql.h> // mysql文件
#include <stdio.h>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>

#define SQL_MAX 99999		// sql语句字符数组最大值

//使用mysql数据库的DAO
class TicketDAO : public ITicketDAO, public BaseDAO
{
public:
    TicketDAO(); // 使用连接池，不需要MYSQL参数
    TicketDAO(MYSQL*); // 兼容旧接口（已废弃）

    //工单相关
    bool createTicket(Ticket& ticket) override;
    bool approveTicket(const Ticket& ticket) override;
    bool dispatchTicket(const Ticket& ticket) override;
    bool completeTicket(const Ticket& ticket) override;



    std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string, std::string>& filter, int offset, int count) override;

    bool orderTransfer(const TicketExecutor& executor) override;
    unsigned long long getOrderCount(const std::map<std::string, std::string>& filter) override;
    std::vector<std::string> getOrderClient() override;
    
    //第一个筛选条件是时间范围（七天一个月或者半年），第二个是工单类型（版本迭代或者交付发送），第三个是客户的名字（可以为空也可以有多个）
    std::vector<std::vector<std::pair<std::string,int>>> selectOrderStatisticsByCondition(int,std::string,std::vector<std::string>) override;

    // 新增：版本分页查询方法
    std::vector<nlohmann::json> getVersionsWithPagination(const std::string& modelName, int offset, int pageSize) override;
    unsigned long long getVersionsCount(const std::string& modelName) override;

    // 新增：基于版本列表的工单查询方法
    std::vector<nlohmann::json> getWorkOrdersWithDetailsByVersions(const std::string& modelName, const std::vector<std::string>& versions) override;

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