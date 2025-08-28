#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include <mysql.h>

#include "UserDAO.h"
#include "UserService.h"
#include "TicketDAO.h"
#include "util/Logger.h"


std::map<int,std::string> id_name;
MYSQL* mysql;
auto ticketDAO = std::make_shared<TicketDAO>(mysql);
auto userDAO = std::make_shared<UserDAO>(mysql);
auto userSvc = std::make_shared<UserService>(userDAO, ticketDAO);

TEST_CASE("get order role") {
    auto ret = userSvc->getOrderRole();
}

TEST_CASE("get modeversion is name") {
    auto ret = userSvc->getUserOrder(666666);

    //auto ret = std::static_pointer_cast<TicketPackage>(userSvc->getUserOrder(666666)[0]);
}

TEST_CASE("login") {

    auto ret = userSvc->login("666666","123456");

    //auto ret = std::static_pointer_cast<TicketPackage>(userSvc->getUserOrder(666666)[0]);
}

TEST_CASE("select myorder") {

    	// 初始化数据库连接池
	if (!DBConnectionManager::initializePool()) {
		LOG_ERROR("数据库连接池初始化失败，程序退出");
        return;
	}
	LOG_INFO("数据库连接池初始化成功");

    const auto& ret = userSvc->getMyOrder("411136");

}