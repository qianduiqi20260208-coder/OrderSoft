#include "EncryptionKeyController.h"
#include <nlohmann/json.hpp>
#include <jwt_utils.h>

EncryptionKeyController::EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> service)
    : encryptionKeyService_(service)
{
}

void EncryptionKeyController::registerRoutes(crow::SimpleApp& app)
{
    // 注册交付操作路由
    CROW_ROUTE(app, "/encryption-key/delivery").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleDeliveryOperation(req);
        });
    
    // 注册入库操作路由
    CROW_ROUTE(app, "/encryption-key/return").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleReturnOperation(req);
        });
    
    // 注册获取可交付外壳号列表路由
    CROW_ROUTE(app, "/encryption-key/available-shells").methods("GET"_method)
        ([this](const crow::request& req) {
            return handleGetAvailableShellNumbers(req);
        });
    
    // 注册创建授权路由
    CROW_ROUTE(app, "/encryption-key/authorization").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleCreateAuthorization(req);
        });
}

crow::response EncryptionKeyController::handleDeliveryOperation(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    // 解析请求体JSON
    auto body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        return crow::response(400, R"({"status":0,"error":"无效的JSON格式","data":{}})");
    }
    
    // 获取请求参数
    std::string clientName = body.value("clientName", "");
    std::string shellNumber = body.value("shellNumber", "");
    std::string deviceType = body.value("deviceType", "");
    std::string deviceNote = body.value("deviceNote", "");
    
    // 参数验证
    if (clientName.empty() || shellNumber.empty() || deviceType.empty()) {
        return crow::response(400, R"({"status":0,"error":"客户名称、外壳号和设备类型不能为空","data":{}})");
    }
    
    // 调用Service层执行交付操作
    bool success = encryptionKeyService_->deliveryOperation(clientName, shellNumber, deviceType, deviceNote);
    
    // 构建响应
    nlohmann::json response;
    if (success) {
        response = {
            {"status", 1},
            {"error", ""},
            {"data", {{"message", "交付操作成功"}}}
        };
    } else {
        response = {
            {"status", 0},
            {"error", "交付操作失败"},
            {"data", nlohmann::json::object()}
        };
    }
    
    return crow::response{response.dump()};
}

crow::response EncryptionKeyController::handleReturnOperation(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    // 解析请求体JSON
    auto body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        return crow::response(400, R"({"status":0,"error":"无效的JSON格式","data":{}})");
    }
    
    // 获取请求参数
    std::string clientName = body.value("clientName", "");
    std::string shellNumber = body.value("shellNumber", "");
    std::string returnDate = body.value("returnDate", "");
    
    // 参数验证
    if (clientName.empty() || shellNumber.empty() || returnDate.empty()) {
        return crow::response(400, R"({"status":0,"error":"客户名称、外壳号和入库日期不能为空","data":{}})");
    }
    
    // 调用Service层执行入库操作
    bool success = encryptionKeyService_->returnOperation(clientName, shellNumber, returnDate);
    
    // 构建响应
    nlohmann::json response;
    if (success) {
        response = {
            {"status", 1},
            {"error", ""},
            {"data", {{"message", "入库操作成功"}}}
        };
    } else {
        response = {
            {"status", 0},
            {"error", "入库操作失败"},
            {"data", nlohmann::json::object()}
        };
    }
    
    return crow::response{response.dump()};
}

crow::response EncryptionKeyController::handleGetAvailableShellNumbers(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    // 调用Service层获取可交付外壳号列表
    auto availableShells = encryptionKeyService_->getAvailableShellNumbers();
    
    // 构建响应数据
    nlohmann::json shellList = nlohmann::json::array();
    for (const auto& shell : availableShells) {
        shellList.push_back({
            {"id", shell.first},
            {"shellNumber", shell.second}
        });
    }
    
    // 构建响应
    nlohmann::json response = {
        {"status", 1},
        {"error", ""},
        {"data", {
            {"shells", shellList},
            {"count", availableShells.size()}
        }}
    };
    
    return crow::response{response.dump()};
}

bool EncryptionKeyController::checkToken(const crow::request& req)
{
    // 使用全局的token验证函数
    return ::checkToken(req);
}

crow::response EncryptionKeyController::handleCreateAuthorization(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    // 解析请求体JSON
    auto body = nlohmann::json::parse(req.body, nullptr, false);
    if (body.is_discarded()) {
        return crow::response(400, R"({"status":0,"error":"无效的JSON格式","data":{}})");
    }
    
    // 获取请求参数
    std::string clientName = body.value("clientName", "");
    std::string shellNumber = body.value("shellNumber", "");
    std::string authId = body.value("authId", "");
    std::string authType = body.value("authType", "");
    std::string startDate = body.value("startDate", "");
    std::string endDate = body.value("endDate", "");
    std::string authNote = body.value("authNote", "");
    
    // 参数验证
    if (clientName.empty() || shellNumber.empty() || authId.empty() || 
        authType.empty() || startDate.empty() || endDate.empty()) {
        return crow::response(400, R"({"status":0,"error":"客户名称、外壳号、授权ID、授权类型、授权开始日期和授权结束日期不能为空","data":{}})");
    }
    
    // 验证授权类型是否有效
    if (authType != "本地锁" && authType != "网络锁" && authType != "软锁授权") {
        return crow::response(400, R"({"status":0,"error":"授权类型必须是：本地锁、网络锁或软锁授权","data":{}})");
    }
    
    // 调用Service层执行创建授权操作
    bool success = encryptionKeyService_->createAuthorization(clientName, shellNumber, authId, authType, startDate, endDate, authNote);
    
    // 构建响应
    nlohmann::json response;
    if (success) {
        response = {
            {"status", 1},
            {"error", ""},
            {"data", {{"message", "创建授权成功"}}}
        };
    } else {
        response = {
            {"status", 0},
            {"error", "创建授权失败"},
            {"data", nlohmann::json::object()}
        };
    }
    
    return crow::response{response.dump()};
}