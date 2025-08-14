#include "CustomerInfoController.h"
#include <jwt_utils.h>

bool CustomerInfoController::checkToken(const crow::request& req)
{
    // 从请求头中获取token字段
    std::string token = req.get_header_value("token");
    if (token.empty()) {
        return false;
    }
    
    // 使用jwt_utils中的全局checkToken函数验证token
    return ::checkToken(req);
}

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
    
    // 注册获取指定客户和外壳号授权信息路由
    CROW_ROUTE(app, "/customer-info/shell-auth").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleGetShellAuthorizationInfo(req);
        });
}

std::string CustomerInfoController::getClientAuthInfo(const std::string& clientName)
{
    // 调用Service层获取客户信息和加密狗授权信息，并转换为JSON格式
    nlohmann::json result = customerInfoService_->getClientAuthInfoJson(clientName);
    
    // 将JSON对象转换为字符串返回
    return result.dump();
}

crow::response CustomerInfoController::handleGetShellAuthorizationInfo(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    try {
        // 解析JSON请求体
        nlohmann::json requestJson = nlohmann::json::parse(req.body);
        
        // 参数验证
        if (!requestJson.contains("clientName") || !requestJson.contains("shellNumber")) {
            return crow::response(400, R"({"status":0,"error":"缺少必要参数clientName或shellNumber","data":{}})");
        }
        
        std::string clientName = requestJson["clientName"];
        std::string shellNumber = requestJson["shellNumber"];
        
        if (clientName.empty() || shellNumber.empty()) {
            return crow::response(400, R"({"status":0,"error":"客户名称和外壳号不能为空","data":{}})");
        }
        
        // 调用Service层获取指定外壳号的授权信息
        nlohmann::json result = customerInfoService_->getShellAuthorizationInfoJson(clientName, shellNumber);
        
        // 设置响应头
        crow::response response(200, result.dump());
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    }
    catch (const nlohmann::json::parse_error& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", "JSON解析失败: " + std::string(e.what())},
            {"data", nlohmann::json::object()}
        };
        return crow::response(400, errorResponse.dump());
    }
    catch (const std::exception& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", std::string("获取外壳号授权信息失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
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