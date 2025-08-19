#include "TicketService.h"

TicketService::TicketService(std::shared_ptr<ITicketDAO> sp1,std::shared_ptr<IModelDAO> sp2):ticketDAO(sp1),modelDAO(sp2)
{
    
}

bool TicketService::createTicket(Ticket &ticket)
{
    return ticketDAO->createTicket(ticket);
}

bool TicketService::approveTicket(const Ticket &ticket)
{
    return ticketDAO->approveTicket(ticket);
}

bool TicketService::dispatchTicket(const Ticket &ticket)
{
    return ticketDAO->dispatchTicket(ticket);
}

bool TicketService::completeTicket(const Ticket &ticket)
{
    bool ret = true;
    //一方面增加新的模型版本，另一方面往工单表里插入数据
    if(ticket.ticketType == "功能开发")
    {
        auto& t = dynamic_cast<const TicketFeature&>(ticket);
        ret = modelDAO->addModelVersion(ticket.model,t.newModelVersion);
    }else if(ticket.ticketType == "版本迭代"){
        auto& t = dynamic_cast<const TicketVersion&>(ticket);
        ret = modelDAO->addModelVersion(ticket.model,t.newModelVersion);
    }else if(ticket.ticketType == "直接封装+发送"){
        auto& t = dynamic_cast<const TicketPackage&>(ticket);
        ret = modelDAO->addModelVersion(ticket.model,t.newModelVersion);
    }
    if(!ret)
    {
        return false; // 如果模型版本添加失败，直接返回
    }
    
    return ticketDAO->completeTicket(ticket);
}



// std::vector<std::shared_ptr<Ticket>> TicketService::selectOrderByCondition(const std::map<std::string, std::string> filter)
// {
//     return ticketDAO->selectOrderByCondition(filter);
// }

std::vector<std::shared_ptr<Ticket>> TicketService::selectOrderByCondition_(const std::map<std::string, std::string> filter, int offset, int pageSize)
{
    return ticketDAO->selectOrderByCondition_(filter, offset, pageSize);
}

bool TicketService::orderTransfer(const TicketExecutor &executor)
{
    return ticketDAO->orderTransfer(executor);
}

unsigned long long TicketService::getOrderCount()
{
    return ticketDAO->getOrderCount();
}

std::vector<std::string> TicketService::getClient()
{
    return ticketDAO->getOrderClient();
}



std::vector<std::pair<std::string, std::string>> TicketService::getVersionsWithPagination(const std::string& modelName, int offset, int pageSize)
{
    auto versionJsons = ticketDAO->getVersionsWithPagination(modelName, offset, pageSize);
    std::vector<std::pair<std::string, std::string>> result;
    
    for (const auto& versionJson : versionJsons) {
        std::string version = versionJson.contains("version") ? versionJson["version"].get<std::string>() : "";
        std::string updateTime = versionJson.contains("updateTime") ? versionJson["updateTime"].get<std::string>() : "";
        result.emplace_back(version, updateTime);
    }
    
    return result;
}

unsigned long long TicketService::getVersionsCount(const std::string& modelName)
{
    return ticketDAO->getVersionsCount(modelName);
}

std::vector<nlohmann::json> TicketService::getWorkOrdersWithDetailsByVersions(const std::string& modelName, const std::vector<std::string>& versions)
{
    return ticketDAO->getWorkOrdersWithDetailsByVersions(modelName, versions);
}

std::vector<std::vector<std::pair<std::string,int>>> TicketService::getOrderStatisticsByCondition(int time_range, std::string ticketType, std::vector<std::string> clientName)
{
    

    return ticketDAO->selectOrderStatisticsByCondition(time_range,ticketType,clientName);
    
}