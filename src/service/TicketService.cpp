#include "TicketService.h"
#include "Logger.h"
#include "WebSocketManager.h"
#ifdef _WIN32
#include <winsock2.h>
#endif

#include <crow/websocket.h>
#include <crow/json.h>



// 工具函数 获取当前时间（本地时间，格式 ISO-like）
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");  // ISO风格
    return oss.str();
}


TicketService::TicketService(std::shared_ptr<ITicketDAO> sp1,std::shared_ptr<IModelDAO> sp2):ticketDAO(sp1),modelDAO(sp2)
{
    
}

bool TicketService::createTicketAndNotify(Ticket &ticket)
{
    //创建工单
    bool b = ticketDAO->createTicket(ticket);

    //通知功能部分 1.封装json
    std::string nowStr = getCurrentTime();
    crow::json::wvalue msg({
    {"type", "todo_notification"},
    {"timestamp", nowStr},
    {"userId", ticket.creatorId},
    {"data", crow::json::wvalue({
        {"id", ticket.id},
        {"title", "新的待办事项"},
        {"orderType", ticket.ticketType},
        {"modelName", ticket.model},
        {"priority", "low"},
        {"status", "pending"},
        {"createdAt", nowStr},
        {"category", "待审批"},
        {"userId", ticket.approverId}
    })}
    });

    //通知功能部分 2.发送消息并把消息持久化
    WebSocketManager::sendToUserAndSaveMsg({(long)stoi(ticket.approverId)}, msg);
    
    return b;
}

bool TicketService::approveTicketAndNotify(const Ticket &ticket)
{
    //如果审批拒绝 就无需通知
    if(ticket.priorityHint != "")
    {

        //通知功能部分 1.封装json
        std::string nowStr = getCurrentTime();

        //如何获取到工单模型这些信息 从数据库中查询工单表获取信息进行联合封装
        auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);

        crow::json::wvalue msg({
        {"type", "todo_notification"},
        {"timestamp", nowStr},
        {"userId", ticket.approverId},
        {"data", crow::json::wvalue({
            {"id", ticket.id},
            {"title", "新的待办事项"},
            {"orderType", queryTicket.ticketType},
            {"modelName", queryTicket.model},
            {"priority", "low"},
            {"status", "pending"},
            {"createdAt", queryTicket.createTime},
            {"category", "待分发"},
            {"userId", ticket.distributorId}
        })}
        });

        //通知功能部分 2.发送消息并把消息持久化
        WebSocketManager::sendToUserAndSaveMsg({(long)stoi(ticket.distributorId)}, msg);
    }

    return ticketDAO->approveTicket(ticket);
}

bool TicketService::dispatchTicketAndNotify(const Ticket &ticket, const std::string& account)
{
    bool b = ticketDAO->dispatchTicket(ticket, account);
    //如果分发拒绝 就无需通知
    if(ticket.priorityTask != "")
    {
        //通知功能部分 1.封装json
        std::string nowStr = getCurrentTime();

        //如何获取到工单模型这些信息 从数据库中查询工单表获取信息进行联合封装
        auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);
        
        crow::json::wvalue msg({
        {"type", "todo_notification"},
        {"timestamp", nowStr},
        {"userId", ticket.distributorId},
        {"data", crow::json::wvalue({
            {"id", ticket.id},
            {"title", "新的待办事项"},
            {"orderType", queryTicket.ticketType},
            {"modelName", queryTicket.model},
            {"priority", "low"},
            {"status", "pending"},
            {"createdAt", queryTicket.createTime},
            {"category", queryTicket.status},
            {"userId", ticket.executorId}
        })}
        });
        
        //通知功能部分 2.发送消息并把消息持久化
        WebSocketManager::sendToUserAndSaveMsg({(long)stoi(ticket.executorId)}, msg);

    }

    return b;
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

    if(ticket.ticketType != "交付发送" && ticket.ticketType != "直接封装+发送")
    {
        //完成工单的人只有一个，但是需要发送给的人有好几个
        //查询流转人的姓名
        std::string username = WebSocketManager::queryNameById(stoi(ticket.executorId));

        //通知功能部分 1.封装json
        std::string nowStr = getCurrentTime();

        //如何获取到工单模型这些信息 从数据库中查询工单表获取信息进行联合封装
        auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);
        
        //查询出需要通知的一些人
        std::vector<long> receivers;
        if(queryTicket.creatorId != "")
        {
            receivers.push_back(stoi(queryTicket.creatorId));
        }
        if(queryTicket.approverId != "")
        {
            receivers.push_back(stoi(queryTicket.approverId));
        }
        if(queryTicket.distributorId != "")
        {
            receivers.push_back(stoi(queryTicket.distributorId));
        }

        //查询客户
        std::string client = WebSocketManager::queryClientByWorkOrderId(ticket.id);

        crow::json::wvalue msg({
        {"type", "todo_notification"},
        {"timestamp", nowStr},
        {"userId", username},
        {"data", crow::json::wvalue({
            {"id", ticket.id},
            {"title", "新的待办事项"},
            {"orderType", queryTicket.ticketType},
            {"modelName", queryTicket.model},
            {"priority", "low"},
            {"status", "pending"},
            {"createdAt", queryTicket.createTime},
            {"category", queryTicket.status},
            {"client",client}
        })}
        });
        
        //通知功能部分 2.发送消息并把消息持久化
        WebSocketManager::sendToUserAndSaveMsg(receivers, msg);
    }
    return ticketDAO->completeTicket(ticket);
}

bool TicketService::completeConcreteTicket(const Ticket& ticket)
{

    bool b = ticketDAO->completeTicket(ticket);

    //除了这这两种工单之外，其他的工单类型都完成了
    if(ticket.ticketType != "交付发送" && ticket.ticketType != "直接封装+发送")
    {
        //完成工单的人只有一个，但是需要发送给的人有好几个
        //查询流转人的姓名
        std::string username = WebSocketManager::queryNameById(stoi(ticket.executorId));

        //通知功能部分 1.封装json
        std::string nowStr = getCurrentTime();

        //查询出需要通知的一些人
        std::vector<long> receivers;
        auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);
        if(queryTicket.creatorId != "")
        {
            receivers.push_back(stoi(queryTicket.creatorId));
        }
        if(queryTicket.approverId != "")
        {
            receivers.push_back(stoi(queryTicket.approverId));
        }
        if(queryTicket.distributorId != "")
        {
            receivers.push_back(stoi(queryTicket.distributorId));
        }

        //查询客户
        std::string client = WebSocketManager::queryClientByWorkOrderId(ticket.id);

        crow::json::wvalue msg({
        {"type", "todo_notification"},
        {"timestamp", nowStr},
        {"userId", username},
        {"data", crow::json::wvalue({
            {"id", ticket.id},
            {"title", "新的待办事项"},
            {"orderType", queryTicket.ticketType},
            {"modelName", queryTicket.model},
            {"priority", "low"},
            {"status", "pending"},
            {"createdAt", queryTicket.createTime},
            {"category", queryTicket.status},
            {"client",client}
        })}
        });
        
        //通知功能部分 2.发送消息并把消息持久化
        WebSocketManager::sendToUserAndSaveMsg(receivers, msg);
    }
    return b;
}

bool TicketService::completeSendTicket(const Ticket& ticket)
{
    bool b = ticketDAO->completeSendTicket(ticket);

    //查询流转人的姓名
    std::string username = WebSocketManager::queryNameById(stoi(ticket.executorId));

    //通知功能部分 1.封装json
    std::string nowStr = getCurrentTime();

    //查询出需要通知的一些人
    auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);
    std::vector<long> receivers;
    if(queryTicket.creatorId != "")
    {
        receivers.push_back(stoi(queryTicket.creatorId));
    }
    if(queryTicket.approverId != "")
    {
        receivers.push_back(stoi(queryTicket.approverId));
    }
    if(queryTicket.distributorId != "")
    {
        receivers.push_back(stoi(queryTicket.distributorId));
    }

    //查询客户
    std::string client = WebSocketManager::queryClientByWorkOrderId(ticket.id);

    crow::json::wvalue msg({
    {"type", "todo_notification"},
    {"timestamp", nowStr},
    {"userId", username},
    {"data", crow::json::wvalue({
        {"id", ticket.id},
        {"title", "新的待办事项"},
        {"orderType", queryTicket.ticketType},
        {"modelName", queryTicket.model},
        {"priority", "low"},
        {"status", "pending"},
        {"createdAt", queryTicket.createTime},
        {"category", queryTicket.status},
        {"client", client},
    })}
    });
    
    //通知功能部分 2.发送消息并把消息持久化
    WebSocketManager::sendToUserAndSaveMsg(receivers, msg);

    return b;
}

bool TicketService::completePackageSendTicket(const TicketPackage& ticket)
{   
    bool b = ticketDAO->completePackageSendTicket(ticket);

    //完成工单的人只有一个，但是需要发送给的人有好几个
    //查询流转人的姓名
    std::string username = WebSocketManager::queryNameById(stoi(ticket.executorId));

    //通知功能部分 1.封装json
    std::string nowStr = getCurrentTime();

    //如何获取到工单模型这些信息 从数据库中查询工单表获取信息进行联合封装
    auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);


    crow::json::wvalue msg({
    {"type", "todo_notification"},
    {"timestamp", nowStr},
    {"userId", username},
    {"data", crow::json::wvalue({
        {"id", ticket.id},
        {"title", "新的待办事项"},
        {"orderType", queryTicket.ticketType},
        {"modelName", queryTicket.model},
        {"priority", "low"},
        {"status", "pending"},
        {"createdAt", queryTicket.createTime},
        {"category", queryTicket.status},
        {"userId", ticket.executorId}
    })}
    });
    
    //通知功能部分 2.发送消息并把消息持久化
    WebSocketManager::sendToUserAndSaveMsg({(long)stoi(ticket.encryptedExecutorId)}, msg);

    return b;
}

bool TicketService::completePackageSendEncryptedTicket(const TicketPackage& ticket)
{
    bool b =ticketDAO->completePackageSendEncryptedTicket(ticket);

    //完成工单的人只有一个，但是需要发送给的人有好几个
    //查询流转人的姓名
    std::string username = WebSocketManager::queryNameById(stoi(ticket.executorId));

    //通知功能部分 1.封装json
    std::string nowStr = getCurrentTime();

    //如何获取到工单模型这些信息 从数据库中查询工单表获取信息进行联合封装
    auto queryTicket = WebSocketManager::queryWorkOrderInfo(ticket.id);

    crow::json::wvalue msg({
    {"type", "todo_notification"},
    {"timestamp", nowStr},
    {"userId", username},
    {"data", crow::json::wvalue({
        {"id", ticket.id},
        {"title", "新的待办事项"},
        {"orderType", queryTicket.ticketType},
        {"modelName", queryTicket.model},
        {"priority", "low"},
        {"status", "pending"},
        {"createdAt", queryTicket.createTime},
        {"category", queryTicket.status},
        {"userId", ticket.executorId}
    })}
    });
    
    //通知功能部分 2.发送消息并把消息持久化
    WebSocketManager::sendToUserAndSaveMsg({(long)stoi(ticket.sendExecutorId)}, msg);

    return b;
}



// std::vector<std::shared_ptr<Ticket>> TicketService::selectOrderByCondition(const std::map<std::string, std::string> filter)
// {
//     return ticketDAO->selectOrderByCondition(filter);
// }

std::vector<std::shared_ptr<Ticket>> TicketService::selectOrderByCondition_(const std::map<std::string, std::string> filter, int offset, int pageSize)
{
    return ticketDAO->selectOrderByCondition_(filter, offset, pageSize);
}

std::vector<nlohmann::json> TicketService::selectOrderByConditionWithDetails(const std::map<std::string, std::string> filter, int offset, int pageSize)
{
    return ticketDAO->selectOrderByConditionWithDetails(filter, offset, pageSize);
}

bool TicketService::orderTransferAndNotify(const TicketTranfer &executor)
{
    bool b = ticketDAO->orderTransfer(executor);
    //通知功能部分 1.封装json
    std::string nowStr = getCurrentTime();

    //如何获取到工单模型这些信息 从数据库中查询工单表获取信息进行联合封装
    auto queryTicket = WebSocketManager::queryWorkOrderInfo(executor.ticketId);
    
    //查询流转人的姓名
    std::string username = WebSocketManager::queryNameById(stoi(executor.createId[0]));

    crow::json::wvalue msg({
    {"type", "todo_notification"},
    {"timestamp", nowStr},
    {"userId", username},
    {"data", crow::json::wvalue({
        {"id", executor.ticketId},
        {"title", "新的待办事项"},
        {"orderType", queryTicket.ticketType},
        {"modelName", queryTicket.model},
        {"priority", "low"},
        {"status", "pending"},
        {"createdAt", queryTicket.createTime},
        {"category", queryTicket.status},
        {"userId", executor.executor[0]}
    })}
    });
    

    //通知功能部分 2.发送消息并把消息持久化
    WebSocketManager::sendToUserAndSaveMsg({(long)stoi(executor.executor[0])}, msg);

    return b;
}

unsigned long long TicketService::getOrderCount(const std::map<std::string, std::string>& filter)
{
    return ticketDAO->getOrderCount(filter);
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

std::vector<nlohmann::json> TicketService::getUserPendingWorkOrders(const std::string& userId)
{
    return ticketDAO->getUserPendingWorkOrders(userId);
}

std::vector<std::vector<std::pair<std::string,int>>> TicketService::getOrderStatisticsByCondition(int time_range, std::string ticketType, std::vector<std::string> clientName)
{
    

    return ticketDAO->selectOrderStatisticsByCondition(time_range,ticketType,clientName);
    
}