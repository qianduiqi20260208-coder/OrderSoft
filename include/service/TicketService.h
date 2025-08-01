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

    //分页查询工单
    std::vector<std::shared_ptr<Ticket>> ticketList(int currentPage,int pageSize) override;
    //根据条件筛选工单
    std::vector<std::shared_ptr<Ticket>> selectOrderByCondition(const std::map<std::string,std::string> filter) override;

    //工单流转的实现
    bool orderTransfer(const TicketExecutor& executor) override;
private:

    std::shared_ptr<ITicketDAO> ticketDAO;
};