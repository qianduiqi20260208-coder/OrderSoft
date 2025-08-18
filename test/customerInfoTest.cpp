#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include <mysql.h>

#include "CustomerInfoDAO.h"
#include "CustomerInfoService.h"


MYSQL* mysql;
auto customerInfoDAO = std::make_shared<CustomerInfoDAO>(mysql);

auto customerInfoSvc = std::make_shared<CustomerInfoService>(customerInfoDAO);

TEST_CASE("customer goods count") {
    auto ret =  customerInfoSvc->getClientGoodsCount();
}


TEST_CASE("getSendRecordByClientPagedByDate") {
    const auto& ret = customerInfoSvc->getSendRecordByClientPagedByDate("华为",0,4);
}

TEST_CASE("getAllModelLatestVesrionByClient") {
    const auto& ret = customerInfoSvc->getAllModelLatestVesrionByClient("华为");
}
