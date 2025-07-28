#pragma once

#include "ITicketService.h"
#include "ITicketDAO.h"
#include <memory>

class TicketService:public ITicketService
{
public:
    TicketService(std::shared_ptr<ITicketDAO>);
    bool createTicket(Ticket& ticket) override;
    bool approveTicket(const Ticket& ticket)override;
    bool dispatchTicket(const Ticket& ticket)override;
    bool completeTicket(const Ticket& ticket)override;

private:

    std::shared_ptr<ITicketDAO> ticketDAO;
};