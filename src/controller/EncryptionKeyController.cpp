#include "EncryptionKeyController.h"
#include <nlohmann/json.hpp>
#include <jwt_utils.h>

EncryptionKeyController::EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> service)
    : encryptionKeyService_(service)
{
}

void EncryptionKeyController::registerRoutes(crow::SimpleApp& app)
{
    // 交付操作路由
    CROW_ROUTE(app, "/encryption-key/delivery").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleDeliveryOperation(req);
        });
    
    // 入库操作路由
    CROW_ROUTE(app, "/encryption-key/return").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleReturnOperation(req);
        });
    
    // 获取可交付外壳号列表路由
    CROW_ROUTE(app, "/encryption-key/available-shells").methods("GET"_method)
        ([this](const crow::request& req) {
            return handleGetAvailableShellNumbers(req);
        });
    
    // 新建授权
    CROW_ROUTE(app, "/encryption-key/authorization").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleCreateAuthorization(req);
        });
    
    // 更新授权截止日期
    CROW_ROUTE(app, "/encryption-key/authorization/update-end-dates").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleUpdateAuthorizationEndDates(req);
        });
    
    // 更新外壳号设备信息
    CROW_ROUTE(app, "/encryption-key/shell/update-device-info").methods("POST"_method)
        ([this](const crow::request& req) {
            return handleUpdateShellDeviceInfo(req);
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

crow::response EncryptionKeyController::handleUpdateShellDeviceInfo(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    try {
        // 解析请求体JSON
        nlohmann::json requestJson = nlohmann::json::parse(req.body);
        
        // 参数验证
        if (!requestJson.contains("clientName") || !requestJson["clientName"].is_string() ||
            !requestJson.contains("shellNumber") || !requestJson["shellNumber"].is_string() ||
            !requestJson.contains("deviceType") || !requestJson["deviceType"].is_string() ||
            !requestJson.contains("deviceNote") || !requestJson["deviceNote"].is_string()) {
            return crow::response(400, R"({"status":0,"error":"参数错误：缺少必要参数或参数类型不正确","data":{}})");
        }
        
        std::string clientName = requestJson["clientName"];
        std::string shellNumber = requestJson["shellNumber"];
        std::string deviceType = requestJson["deviceType"];
        std::string deviceNote = requestJson["deviceNote"];
        
        // 参数非空验证
        if (clientName.empty() || shellNumber.empty() || deviceType.empty()) {
            return crow::response(400, R"({"status":0,"error":"客户名称、外壳号和设备类型不能为空","data":{}})");
        }
        
        // 调用服务层更新外壳号设备信息
        bool success = encryptionKeyService_->updateShellDeviceInfo(clientName, shellNumber, deviceType, deviceNote);
        
        if (success) {
            nlohmann::json responseJson = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"message", "外壳号设备信息更新成功"},
                    {"clientName", clientName},
                    {"shellNumber", shellNumber},
                    {"deviceType", deviceType}
                }}
            };
            return crow::response(200, responseJson.dump());
        } else {
            return crow::response(400, R"({"status":0,"error":"更新外壳号设备信息失败","data":{}})");
        }
    }
    catch (const nlohmann::json::parse_error& e) {
        return crow::response(400, R"({"status":0,"error":"JSON格式错误","data":{}})");
    }
    catch (const std::exception& e) {
        return crow::response(500, R"({"status":0,"error":"服务器内部错误","data":{}})");
    }
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

crow::response EncryptionKeyController::handleUpdateAuthorizationEndDates(const crow::request& req)
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
    if (!body.contains("clientName") || !body.contains("changes")) {
        return crow::response(400, R"({"status":0,"error":"缺少必要参数clientName或changes","data":{}})");
    }
    
    std::string clientName = body["clientName"];
    auto changesJson = body["changes"];
    
    // 验证changes是数组
    if (!changesJson.is_array()) {
        return crow::response(400, R"({"status":0,"error":"changes参数必须是数组","data":{}})");
    }
    
    // 解析changes数组
    std::vector<std::tuple<std::string, std::string, std::string>> changes;
    for (const auto& change : changesJson) {
        if (!change.contains("shellNumber") || !change.contains("authId") || !change.contains("newEndDate")) {
            return crow::response(400, R"({"status":0,"error":"changes数组中的每个元素必须包含shellNumber、authId和newEndDate","data":{}})");
        }
        
        std::string shellNumber = change["shellNumber"];
        std::string authId = change["authId"];
        std::string newEndDate = change["newEndDate"];
        
        changes.emplace_back(shellNumber, authId, newEndDate);
    }
    
    // 调用服务层方法
    bool success = encryptionKeyService_->updateAuthorizationEndDates(clientName, changes);
    
    // 构造响应
    nlohmann::json response;
    if (success) {
        response = {
            {"status", 1},
            {"error", ""},
            {"data", {{"message", "批量更新授权截止日期成功"}, {"updatedCount", changes.size()}}}
        };
    } else {
        response = {
            {"status", 0},
            {"error", "批量更新授权截止日期失败"},
            {"data", nlohmann::json::object()}
        };
    }

    return crow::response{response.dump()};
}