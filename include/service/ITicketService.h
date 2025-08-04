#pragma once
#include "Entity.h"
#include <string>
#include <map>
#include <memory>

class ITicketService
{
public:
    virtual bool createTicket(Ticket& ticket){return true;};
    virtual bool approveTicket(const Ticket& ticket){return true;};
    virtual bool dispatchTicket(const Ticket& ticket){return true;};
    virtual bool completeTicket(const Ticket& ticket){return true;};


    virtual std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string,std::string> filter, int offset, int pageSize) = 0;

    virtual bool orderTransfer(const TicketExecutor& executor) = 0;
    virtual ~ITicketService() =0;
    
};

inline ITicketService::~ITicketService() = default;