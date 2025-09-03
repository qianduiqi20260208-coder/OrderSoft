#pragma once
#include "Entity.h"
#include <string>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

class ITicketService
{
protected:
public:
    virtual bool createTicket(Ticket& ticket){return true;};
    virtual bool approveTicket(const Ticket& ticket){return true;};
    virtual bool dispatchTicket(const Ticket& ticket, const std::string& account){return true;};
    virtual bool completeTicket(const Ticket& ticket){return true;};
    virtual bool completeConcreteTicket(const Ticket& ticket){return true;};
    virtual bool completeSendTicket(const Ticket& ticket){return true;};
    virtual bool completePackageSendTicket(const TicketPackage& ticket){return true;};
    virtual bool completePackageSendEncryptedTicket(const TicketPackage& ticket){return true;};


    virtual std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string,std::string> filter, int offset, int pageSize) = 0;

    // 新增：复杂工单查询方法（支持分页）
    virtual std::vector<nlohmann::json> selectOrderByConditionWithDetails(const std::map<std::string,std::string> filter, int offset, int pageSize) = 0;

    
    // 新增：版本分页查询方法
    virtual std::vector<std::pair<std::string, std::string>> getVersionsWithPagination(const std::string& modelName, int offset, int pageSize) = 0;
    virtual unsigned long long getVersionsCount(const std::string& modelName) = 0;
    
    // 新增：基于版本列表的工单查询方法
    virtual std::vector<nlohmann::json> getWorkOrdersWithDetailsByVersions(const std::string& modelName, const std::vector<std::string>& versions) = 0;

    // 新增：获取用户待办工单（包含详细信息和流转信息）
    virtual std::vector<nlohmann::json> getUserPendingWorkOrders(const std::string& userId) = 0;

    virtual bool orderTransfer(const TicketTranfer& executor) = 0;
    virtual unsigned long long getOrderCount(const std::map<std::string, std::string>& filter) = 0;

    virtual std::vector<std::string> getClient() = 0;

    virtual std::vector<std::vector<std::pair<std::string,int>>> getOrderStatisticsByCondition(int,std::string,std::vector<std::string>) = 0;

    virtual ~ITicketService() =0;
    
};

inline ITicketService::~ITicketService() = default;