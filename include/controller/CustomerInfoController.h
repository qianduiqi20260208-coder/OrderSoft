/*****************************************************************//**
 * \file   CustomerInfoController.h
 * \brief  客户信息控制器，负责注册客户相关的 HTTP 路由
 * 
 * \author 
 * \date   August 2025
 *********************************************************************/
#pragma once
#include "ICustomerInfoService.h"
#include "entity/Entity.h"
#include "util/ticket.h"
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

class CustomerInfoController {
public:
    // CustomerInfoController(std::shared_ptr<ICustomerInfoService>);
    CustomerInfoController();
    /**
     * @brief 注册路由到 Crow 应用
     * @param app Crow 应用对象
     */
    void registerRoutes(crow::SimpleApp& app);
private:
    // std::shared_ptr<ICustomerInfoService> modelService;
};