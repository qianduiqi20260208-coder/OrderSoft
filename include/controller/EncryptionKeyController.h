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
    
private:
    std::shared_ptr<IEncryptionKeyService> encryptionKeyService_;
};