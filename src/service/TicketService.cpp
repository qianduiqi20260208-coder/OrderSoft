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

// std::vector<std::shared_ptr<Ticket>> TicketService::ticketList(int currentPage, int pageSize)
// {
//     //将currentPage和pageSize转换成数据库需要字段
//     int offset = currentPage * pageSize;
//     int count = pageSize;

//     return ticketDAO->ticketList(offset,count);
// }

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
