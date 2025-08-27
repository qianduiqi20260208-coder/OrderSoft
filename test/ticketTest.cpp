#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include "TicketDAO.h"
#include "TicketService.h"
#include <mysql.h>
#include "ModelDAO.h"
#include "ModelService.h"
#include "UserDAO.h"
#include "UserService.h"

std::map<int,std::string> id_name;


MYSQL* mysql;
auto ticketDAO = std::make_shared<TicketDAO>(mysql);
auto modelDAO = std::make_shared<ModelDAO>(mysql);
auto ticketSvc = std::make_shared<TicketService>(ticketDAO,modelDAO);

TEST_CASE("approver and dispatcher are one person") {
    TicketReproduce ticket;
	ticket.Ticket::id =1;
	ticket.approverId = 888888;
	ticket.priorityHint = "紧急";
	ticket.distributorId = 888888;
    CHECK(ticketSvc->approveTicket(ticket));
}

TEST_CASE("approve reject") {
    TicketReproduce ticket;
	ticket.Ticket::id =1;
	ticket.approverId = 888888;
	ticket.distributorId = 888888;
    ticket.rejectReason = "这是拒绝原因";
    CHECK(ticketSvc->approveTicket(ticket));
}


TEST_CASE("order tranfer") {
    TicketExecutor ticketExecutor;
    ticketExecutor.ticketId = 1;
    ticketExecutor.executor.push_back("888888");
    ticketExecutor.executor.push_back("999999");
    ticketExecutor.reason.push_back("我是888888");

    ticketSvc->orderTransfer(ticketExecutor);
}

TEST_CASE("order select by status and model_version_id and page limit") {
    std::map<std::string,std::string> filter;
    filter["status"] = "待审批";
    filter["model_version_id"] = "1";
    auto retVec = ticketSvc->selectOrderByCondition_(filter,0,2);
    auto ret = std::static_pointer_cast<TicketPackage>(retVec[0]);
}

TEST_CASE("no upload attachment") {
    TicketReproduce ticket;

	ticket.creatorId = 666666;
	ticket.approverId = 888888;
	ticket.ticketType = "问题复现";
	ticket.model = "ATA04_Aerodynamics";
	ticket.modelVersion = "12.2.2";

	ticket.coordinationId = "这是协调单号";
	ticket.content = "这是复现内容";

    ticketSvc->createTicket(ticket);
}

TEST_CASE("function dev new model version") {
	TicketFeature ticket;
	ticket.Ticket::id =3;
	ticket.ticketType = "功能开发";
    ticket.model = "ATA04_Aerodynamics";
	ticket.featureFinal = "功能开发最终描述";
    ticket.modelVersion = "12.2.3";
    ticketSvc->completeTicket(ticket);
}

TEST_CASE("select order sum") {
}

TEST_CASE("get client") {

    auto ret = ticketSvc->getClient();
}

TEST_CASE("dispatach reject") {
    TicketReproduce ticket;
    ticket.distributorId = 888888;
	ticket.Ticket::id =1;
    ticket.rejectReason = "拒绝分发";
	ticketSvc->dispatchTicket(ticket);
    
}

TEST_CASE("dispatach approve") {
    TicketReproduce ticket;
	ticket.Ticket::id =1;

    ticket.priorityTask = "紧急";
    ticket.executorId = 666666;

	ticketSvc->dispatchTicket(ticket);
    
}

TEST_CASE("approve encryption") {
	//交付发送类工单
	TicketDelivery ticket;
	ticket.Ticket::id =9;
	ticket.ticketType = "交付发送";
	ticket.encrypted = 1;
	ticket.dongleId = "qwer1234";
	ticket.licenseId = "333444555";
	ticket.remark ="这是交付发送类工单的备注";

	ticketSvc->completeTicket(ticket);
    
}

TEST_CASE("approve encryption2") {
	//直接封装+发送类工单
	TicketPackage ticket;
	ticket.Ticket::id =10;
	ticket.ticketType = "直接封装+发送";
	ticket.model ="ATA04_Aerodynamics";
	ticket.newModelVersion = "12.2.2";
	ticket.encrypted = 1;
	ticket.license = "333444555";
	ticket.dongle = "qwer1234";
	ticket.remark ="这是直接封装+发送类工单的备注";

	ticketSvc->completeTicket(ticket);
    
}

TEST_CASE("reject encryption") {
	//交付发送类工单
	TicketDelivery ticket;
	ticket.Ticket::id =9;
	ticket.ticketType = "交付发送";
	ticket.encrypted = 0;
	ticket.dongleId = "qwer1234";
	ticket.licenseId = "333444555";
	ticket.remark ="这是交付发送类工单的备注";

	ticketSvc->completeTicket(ticket);
    
}

TEST_CASE("reject encryption2") {
	//直接封装+发送类工单
	TicketPackage ticket;
	ticket.Ticket::id =10;
	ticket.ticketType = "直接封装+发送";
	ticket.model ="ATA04_Aerodynamics";
	ticket.newModelVersion = "12.2.2";
	ticket.encrypted = 0;
	ticket.license = "333444555";
	ticket.dongle = "qwer1234";
	ticket.remark ="这是直接封装+发送类工单的备注";

	ticketSvc->completeTicket(ticket);
}

TEST_CASE("order statistics") {
	//查询近七天的版本迭代工单
	auto ret1 = ticketSvc->getOrderStatisticsByCondition(0,"版本迭代",{""});
    
	//查询最近一周的交付发送类工单
	auto ret2 = ticketSvc->getOrderStatisticsByCondition(0,"交付发送",{"华为","zhangsan"});
}
