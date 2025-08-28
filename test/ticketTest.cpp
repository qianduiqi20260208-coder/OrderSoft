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
#include "Logger.h"

std::map<int,std::string> id_name;


MYSQL* mysql;
auto ticketDAO = std::make_shared<TicketDAO>(mysql);
auto modelDAO = std::make_shared<ModelDAO>(mysql);
auto ticketSvc = std::make_shared<TicketService>(ticketDAO,modelDAO);


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
    filter["target_delivery_time"] = "2025-08-27";
	// 初始化数据库连接池
	if (!DBConnectionManager::initializePool()) {
		LOG_ERROR("数据库连接池初始化失败，程序退出");
	}
	LOG_INFO("数据库连接池初始化成功");

    auto retVec = ticketSvc->selectOrderByCondition_(filter,0,2);
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


TEST_CASE("order statistics") {
	//查询近七天的版本迭代工单
	auto ret1 = ticketSvc->getOrderStatisticsByCondition(0,"版本迭代",{""});
    
	//查询最近一周的交付发送类工单
	auto ret2 = ticketSvc->getOrderStatisticsByCondition(0,"交付发送",{"华为","zhangsan"});
}
