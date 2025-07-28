#pragma once
#include "Entity.h"
#include <vector>
#include <memory>

//DAO层公用的接口
class ITicketDAO
{
public:

    virtual bool createTicket(Ticket& ticket){return true;};
    virtual bool approveTicket(const Ticket& ticket){return true;};
    virtual bool dispatchTicket(const Ticket& ticket){return true;};
    virtual bool completeTicket(const Ticket& ticket){return true;};

    virtual ~ITicketDAO() = 0;

};

inline ITicketDAO::~ITicketDAO() = default;