#pragma once
#include "../entity/Entity.h"
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

#include <string>
#include <map>

//DAO层公用的接口
class ITicketDAO
{
public:

    virtual bool createTicket(Ticket& ticket){return true;};
    virtual bool approveTicket(const Ticket& ticket){return true;};
    virtual bool dispatchTicket(const Ticket& ticket, const std::string& account){return true;};
    virtual bool completeTicket(const Ticket& ticket){return true;};
    
    virtual std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string, std::string>& filter, int offset, int count) = 0;

    // 新增：复杂工单查询方法（支持分页）

    
    // 新增：版本分页查询方法
    virtual std::vector<nlohmann::json> getVersionsWithPagination(const std::string& modelName, int offset, int pageSize) = 0;
    virtual unsigned long long getVersionsCount(const std::string& modelName) = 0;
    
    // 新增：基于版本列表的工单查询方法
    virtual std::vector<nlohmann::json> getWorkOrdersWithDetailsByVersions(const std::string& modelName, const std::vector<std::string>& versions) = 0;

    virtual bool orderTransfer(const TicketTranfer& executor) = 0;
    virtual unsigned long long getOrderCount(const std::map<std::string, std::string>& filter) = 0 ;
    virtual std::vector<std::string> getOrderClient() = 0;

    virtual std::vector<std::vector<std::pair<std::string,int>>> selectOrderStatisticsByCondition(int,std::string,std::vector<std::string>) = 0;

    virtual ~ITicketDAO() = 0;

};

inline ITicketDAO::~ITicketDAO() = default;