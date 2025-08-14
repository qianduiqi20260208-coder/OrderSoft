#pragma once
#include "IEncryptionKeyService.h"
#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include <crow.h>

class EncryptionKeyController
{
public:
    EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> service);
    
    // 注册路由
    void registerRoutes(crow::SimpleApp& app);
    
    // 交付操作处理函数
    crow::response handleDeliveryOperation(const crow::request& req);
    
    // 入库操作处理函数
    crow::response handleReturnOperation(const crow::request& req);
    
    // 获取可交付外壳号列表处理函数
    crow::response handleGetAvailableShellNumbers(const crow::request& req);
    
    // 创建授权处理函数
    crow::response handleCreateAuthorization(const crow::request& req);
    
    // 处理更新授权截止日期请求
    crow::response handleUpdateAuthorizationEndDates(const crow::request& req);

    // 处理更新外壳号设备信息请求
    crow::response handleUpdateShellDeviceInfo(const crow::request& req);

private:
    std::shared_ptr<IEncryptionKeyService> encryptionKeyService_;
    
    // 检查token是否有效
    bool checkToken(const crow::request& req);
};