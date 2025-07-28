/*****************************************************************//**
 * \file   ModelController.h
 * \brief  模型版本控制器，负责注册模型相关的 HTTP 路由
 * 
 * \author 
 * \date   July 2025
 *********************************************************************/
#pragma once
#include "IModelService.h"
#include "entity/Entity.h"
#include "util/ticket.h"
#include <memory>
#include <crow.h>
#include <nlohmann/json.hpp>

class ModelController {
public:
    ModelController(std::shared_ptr<IModelService>);
    /**
     * @brief 注册路由到 Crow 应用
     * @param app Crow 应用对象
     */
    void registerRoutes(crow::SimpleApp& app);
private:
    std::shared_ptr<IModelService> modelService;
};