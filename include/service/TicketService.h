#pragma once

#include "ITicketService.h"
#include "ITicketDAO.h"
#include "IModelDAO.h"

#include <memory>

class TicketService:public ITicketService
{
public:
    TicketService(std::shared_ptr<ITicketDAO>,std::shared_ptr<IModelDAO>);
    bool createTicket(Ticket& ticket) override;
    bool approveTicket(const Ticket& ticket)override;
    bool dispatchTicket(const Ticket& ticket)override;
    bool completeTicket(const Ticket& ticket)override;

    //根据条件筛选工单
    std::vector<std::shared_ptr<Ticket>> selectOrderByCondition_(const std::map<std::string,std::string> filter, int offset, int pageSize) override;

    //工单流转的实现
    bool orderTransfer(const TicketExecutor& executor) override;
    unsigned long long getOrderCount() override;
    std::vector<std::string> getClient() override;
private:

    std::shared_ptr<ITicketDAO> ticketDAO;
    std::shared_ptr<IModelDAO>  modelDAO;
};