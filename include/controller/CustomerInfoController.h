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
#include "crow/middlewares/cors.h"
#include <string>
#include <nlohmann/json.hpp>

class CustomerInfoController
{
public:
    CustomerInfoController(std::shared_ptr<ICustomerInfoService> service);

    // 注册路由
    void registerRoutes(crow::App<crow::CORSHandler>& app);

    // 根据客户名称获取客户信息和加密狗授权信息，并返回JSON格式
    std::string getClientAuthInfo(const std::string& clientName);

    // 获取所有客户名称列表，并返回JSON格式
    std::string getAllClientNames();

    // 处理获取客户授权信息的HTTP请求
    crow::response handleGetClientAuthInfo(const crow::request& req, const std::string& clientName);

    // 处理获取所有客户名称的HTTP请求
    crow::response handleGetAllClientNames(const crow::request& req);

    // 处理获取指定客户和外壳号授权信息的HTTP请求
    crow::response handleGetShellAuthorizationInfo(const crow::request& req);

private:
    std::shared_ptr<ICustomerInfoService> customerInfoService_;
};