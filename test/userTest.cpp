#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include <mysql.h>

#include "UserDAO.h"
#include "UserService.h"


MYSQL* mysql;
auto userDAO = std::make_shared<UserDAO>(mysql);
auto userSvc = std::make_shared<UserService>(userDAO);

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