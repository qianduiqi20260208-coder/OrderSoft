#include "TicketService.h"

TicketService::TicketService(std::shared_ptr<ITicketDAO> sp):ticketDAO(sp)
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
    return ticketDAO->completeTicket(ticket);
}
