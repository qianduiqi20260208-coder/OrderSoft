/*****************************************************************//**
 * \file   EncryptionKeyController.h
 * \brief  加密狗控制器，负责注册加密狗相关的 HTTP 路由
 * 
 * \author 
 * \date   August 2025
 *********************************************************************/
#pragma once
#include "IEncryptionKeyService.h"
#include "entity/Entity.h"
#include "util/ticket.h"
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

class EncryptionKeyController {
public:
    EncryptionKeyController(std::shared_ptr<IEncryptionKeyService>);
    /**
     * @brief 注册路由到 Crow 应用
     * @param app Crow 应用对象
     */
    void registerRoutes(crow::SimpleApp& app);
private:
    std::shared_ptr<IEncryptionKeyService> encryptionKeyService;
};