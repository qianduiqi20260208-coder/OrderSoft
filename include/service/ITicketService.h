#pragma once
#include "Entity.h"
#include <memory>

class ITicketService
{
public:
    virtual bool createTicket(Ticket& ticket){return true;};
    virtual bool approveTicket(const Ticket& ticket){return true;};
    virtual bool dispatchTicket(const Ticket& ticket){return true;};
    virtual bool completeTicket(const Ticket& ticket){return true;};

    virtual ~ITicketService() =0;
    
};

inline ITicketService::~ITicketService() = default;