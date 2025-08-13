#include "CustomerInfoController.h"
#include <jwt_utils.h>

CustomerInfoController::CustomerInfoController(std::shared_ptr<ICustomerInfoService> service)
    : customerInfoService_(service)
{
}

void CustomerInfoController::registerRoutes(crow::SimpleApp& app)
{
    // 注册获取客户授权信息路由
    CROW_ROUTE(app, "/customer-info/auth/<string>").methods("GET"_method)
        ([this](const crow::request& req, const std::string& clientName) {
            return handleGetClientAuthInfo(req, clientName);
        });
    
    // 注册获取所有客户名称列表路由
    CROW_ROUTE(app, "/customer-info/clients").methods("GET"_method)
        ([this](const crow::request& req) {
            return handleGetAllClientNames(req);
        });
}

std::string CustomerInfoController::getClientAuthInfo(const std::string& clientName)
{
    // 调用Service层获取客户信息和加密狗授权信息，并转换为JSON格式
    nlohmann::json result = customerInfoService_->getClientAuthInfoJson(clientName);
    
    // 将JSON对象转换为字符串返回
    return result.dump();
}

crow::response CustomerInfoController::handleGetClientAuthInfo(const crow::request& req, const std::string& clientName)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    // 参数验证
    if (clientName.empty()) {
        return crow::response(400, R"({"status":0,"error":"客户名称不能为空","data":{}})");
    }
    
    try {
        // 调用业务逻辑获取客户授权信息
        std::string result = getClientAuthInfo(clientName);
        
        // 设置响应头
        crow::response response(200, result);
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    }
    catch (const std::exception& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", std::string("获取客户授权信息失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
}

crow::response CustomerInfoController::handleGetAllClientNames(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    try {
        // 调用业务逻辑获取所有客户名称
        std::string result = getAllClientNames();
        
        // 设置响应头
        crow::response response(200, result);
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    }
    catch (const std::exception& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", std::string("获取客户名称列表失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
}

std::string CustomerInfoController::getAllClientNames()
{
    // 调用Service层获取所有客户名称列表
    std::vector<std::string> clientNames = customerInfoService_->getAllClientNames();
    
    // 构建返回的JSON数据
    nlohmann::json result;
    result["error"] = "";
    result["status"] = 1;
    result["data"] = nlohmann::json::array();
    
    // 将客户名称列表添加到JSON数据中
    for (const auto& clientName : clientNames)
    {
        result["data"].push_back(clientName);
    }
    
    // 将JSON对象转换为字符串返回
    return result.dump();
}