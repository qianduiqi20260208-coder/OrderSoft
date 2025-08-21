/*****************************************************************//**
 * \file   UserController.h
 * \brief  用户路由控制器，负责注册用户相关的路由
 * 
 * \author 
 * \date   July 2025
 *********************************************************************/
#pragma once
// #include "../service/user_service.h"
#include "../service/UserService.h"
#include "entity/Entity.h"
#include <crow.h>
#include "crow/middlewares/cors.h"
#include <nlohmann/json.hpp>

class UserController
{
public:
    UserController(std::shared_ptr<IUserService>);
    /**
     * @brief 注册路由到 Crow 应用
     * @param app Crow 应用对象
     */
    void registerRoutes(crow::App<crow::CORSHandler>& app);

    /**
     * @brief 将中文角色名称转换为英文
     * @param chineseRole 中文角色名称
     * @return 对应的英文角色名称
     */
    std::string convertRoleToEnglish(const std::string& chineseRole) const;
    std::string convertFlowRoleToEnglish(const std::string& chineseFlowRole) const;
private:
    std::shared_ptr<IUserService> userService;
    std::shared_ptr<UserService> userService_test;

    User user;

};

