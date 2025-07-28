#ifdef _WIN32
#include <winsock2.h>
#endif

#include <memory>
#include <mysql.h>
#include <iostream>
#include <locale>

#include "crow.h" // Crow framework for web applications
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


int main() {

	MYSQL mysql;
	mysql_init(&mysql);

	mysql_set_character_set(&mysql, "utf8mb4");
	
	// 设置控制台为 UTF-8 模式 非常重要！！！
    SetConsoleOutputCP(CP_UTF8);

	UserController userController(std::make_shared<UserService>(std::make_shared<UserDAO>(&mysql)));
	TicketController ticketController(std::make_shared<TicketService>(std::make_shared<TicketDAO>(&mysql)));
	ModelController modelController(std::make_shared<ModelService>(std::make_shared<ModelDAO>(&mysql)));

	crow::SimpleApp app;
    userController.registerRoutes(app);
	ticketController.registerRoutes(app);
	modelController.registerRoutes(app);

	app.port(18080).multithreaded().run();
	

	return 0;
}