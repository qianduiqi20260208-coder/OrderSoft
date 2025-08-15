#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <memory>
#include <mysql.h>

#include "ModelDAO.h"
#include "ModelService.h"
#include "TicketDAO.h"

MYSQL* mysql;
auto modelDAO = std::make_shared<ModelDAO>(mysql);
auto ticketDAO = std::make_shared<TicketDAO>(mysql);
auto modelSvc = std::make_shared<ModelService>(modelDAO,ticketDAO);

TEST_CASE("model version with orders") {

    auto ret = modelSvc->getModelVersionWithOrdersByModelPaged("ATA04_Aerodynamics",0,10);
    auto ret2 = std::static_pointer_cast<TicketVersion>(ret[0].second[0]);

}

TEST_CASE("get model version count") {


    auto ret = modelSvc->getModelVersionCount("ATA04_Aerodynamics");
}