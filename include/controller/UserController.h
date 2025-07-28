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
#include <nlohmann/json.hpp>

class UserController
{
public:
    UserController(std::shared_ptr<IUserService>);
    /**
     * @brief 注册路由到 Crow 应用
     * @param app Crow 应用对象
     */
    void registerRoutes(crow::SimpleApp& app);
private:
    std::shared_ptr<IUserService> userService;
    std::shared_ptr<UserService> userService_test;

    User user;

};

