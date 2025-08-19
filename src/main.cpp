#ifdef _WIN32
#include <winsock2.h>
#endif

#include <memory>
#include <mysql.h>
#include <iostream>
#include <locale>
#include "crow.h"
// 工单模块
#include "TicketDAO.h"
#include "TicketService.h"
#include "TicketController.h"
// 用户模块
#include "UserDAO.h"
#include "UserService.h"	
#include "UserController.h"
// 模型模块
#include "ModelDAO.h"
#include "ModelService.h"
#include "ModelController.h"
// 交付模块
#include "CustomerInfoDAO.h"
#include "CustomerInfoService.h"
#include "CustomerInfoController.h"
#include "EncryptionKeyDAO.h"
#include "EncryptionKeyService.h"
#include "EncryptionKeyController.h"


int main() {

	MYSQL mysql;
	mysql_init(&mysql);

	mysql_set_character_set(&mysql, "utf8mb4");
	
	// 设置控制台为 UTF-8 模式 非常重要！！！
    SetConsoleOutputCP(CP_UTF8);

    // 创建 DAO 对象
    auto userDAO = std::make_shared<UserDAO>(&mysql);
    auto ticketDAO = std::make_shared<TicketDAO>(&mysql);
    auto modelDAO = std::make_shared<ModelDAO>(&mysql);  // 添加 ModelDAO
    auto customerInfoDAO = std::make_shared<CustomerInfoDAO>(&mysql);
    auto encryptionKeyDAO = std::make_shared<EncryptionKey>(&mysql);

    // 创建 Service 对象
    auto userService = std::make_shared<UserService>(userDAO);
    auto ticketService = std::make_shared<TicketService>(ticketDAO, modelDAO);  // 传递两个参数
    auto modelService = std::make_shared<ModelService>(modelDAO, ticketDAO);
    auto customerInfoService = std::make_shared<CustomerInfoService>(customerInfoDAO);
    auto encryptionKeyService = std::make_shared<EncryptionKeyService>(encryptionKeyDAO);

    // 创建 Controller 对象
    UserController userController(userService);
    TicketController ticketController(ticketService);
    ModelController modelController(modelService);
    CustomerInfoController customerInfoController(customerInfoService);
    EncryptionKeyController encryptionKeyController(encryptionKeyService);

	crow::SimpleApp app;
    userController.registerRoutes(app);
	ticketController.registerRoutes(app);
	modelController.registerRoutes(app);
    customerInfoController.registerRoutes(app);
    encryptionKeyController.registerRoutes(app);

	app.port(18080).multithreaded().run();
	

	return 0;
}

