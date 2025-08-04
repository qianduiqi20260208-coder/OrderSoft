#pragma once
#include "Entity.h"
#include <vector>
#include <memory>

#include <string>
#include <map>

//DAO层公用的接口
class ITicketDAO
{
public:

    virtual bool createTicket(Ticket& ticket){return true;};
    virtual bool approveTicket(const Ticket& ticket){return true;};
    virtual bool dispatchTicket(const Ticket& ticket){return true;};
    virtual bool completeTicket(const Ticket& ticket){return true;};

    virtual std::vector<std::shared_ptr<Ticket>> ticketList(int offset,int count) = 0;
    virtual std::vector<std::shared_ptr<Ticket>> selectOrderByCondition(const std::map<std::string,std::string> filter,int offset, int count) = 0;

    virtual bool orderTransfer(const TicketExecutor& executor) = 0;
    virtual ~ITicketDAO() = 0;

};

inline ITicketDAO::~ITicketDAO() = default;