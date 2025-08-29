#pragma once

#include "ITicketService.h"
#include "ITicketDAO.h"
#include "IModelDAO.h"

#include <memory>

class TicketService:public ITicketService
{
public:
    TicketService(std::shared_ptr<ITicketDAO>,std::shared_ptr<IModelDAO>);
    bool createTicket(Ticket& ticket) override;
    bool approveTicket(const Ticket& ticket)override;
    bool dispatchTicket(const Ticket& ticket, const std::string& account)override;
    bool completeTicket(const Ticket& ticket)override;

    //根据条件筛选工单
    std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string,std::string> filter, int offset, int pageSize) override;

    //工单流转的实现
    bool orderTransfer(const TicketTranfer& executor) override;
    unsigned long long getOrderCount(const std::map<std::string, std::string>& filter) override;
    std::vector<std::string> getClient() override;

    //第一个参数： 0是筛选出一周的数据 1是筛选出一个月的数据 2是筛选出半年的数据。 筛选版本迭代工单时需要传入的vector数据是:{""}
    std::vector<std::vector<std::pair<std::string,int>>> getOrderStatisticsByCondition(int,std::string,std::vector<std::string>) override;

    // 新增：版本分页查询方法
    std::vector<std::pair<std::string, std::string>> getVersionsWithPagination(const std::string& modelName, int offset, int pageSize) override;
    unsigned long long getVersionsCount(const std::string& modelName) override;

    // 新增：基于版本列表的工单查询方法
    std::vector<nlohmann::json> getWorkOrdersWithDetailsByVersions(const std::string& modelName, const std::vector<std::string>& versions) override;

    // 新增：获取用户待办工单（包含详细信息和流转信息）
    std::vector<nlohmann::json> getUserPendingWorkOrders(const std::string& userId) override;
private:

    std::shared_ptr<ITicketDAO> ticketDAO;
    std::shared_ptr<IModelDAO>  modelDAO;
};