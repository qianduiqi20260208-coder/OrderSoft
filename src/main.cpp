#ifdef _WIN32
#include <winsock2.h>
#endif

#include <memory>
#include <mysql.h>
#include <iostream>
#include <filesystem>
#include <locale>
#include "crow.h"
#include "crow/middlewares/cors.h"
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
#include "DBConnectionManager.h"
#include "util/LoggerConfig.h"
#include "util/Logger.h"

std::map<int,std::string> id_name;

int main() {

	// 初始化日志系统
	LoggerConfig::initFromConfig("config.ini");
    std::filesystem::path iniPath = std::filesystem::absolute("config.ini");
    std::cout << "INI文件绝对路径: " << iniPath.string() << std::endl;
	LOG_INFO("ModelLifeManager 服务启动中..");

	// 初始化数据库连接池
	if (!DBConnectionManager::initializePool()) {
		LOG_ERROR("数据库连接池初始化失败，程序退出");
		return -1;
	}
	LOG_INFO("数据库连接池初始化成功");
	
	// 设置控制台为 UTF-8 模式 非常重要！！！
    SetConsoleOutputCP(CP_UTF8);

    //一个工号跟姓名之间的映射
    add_idname_mapping();

    // 创建 DAO 对象（现在使用连接池，不需要传递MYSQL对象）
    auto userDAO = std::make_shared<UserDAO>();
    auto ticketDAO = std::make_shared<TicketDAO>();
    auto modelDAO = std::make_shared<ModelDAO>();  // 使用连接池
    auto customerInfoDAO = std::make_shared<CustomerInfoDAO>();
    auto encryptionKeyDAO = std::make_shared<EncryptionKey>();

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

	crow::App<crow::CORSHandler> app;

    // 使用Crow内置的CORS处理器
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors.global()
        .headers("Content-Type", "Authorization", "X-Requested-With", "token")
        .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method, "OPTIONS"_method)
        .origin("*");

    
    userController.registerRoutes(app);
	ticketController.registerRoutes(app);
	modelController.registerRoutes(app);
    customerInfoController.registerRoutes(app);
    encryptionKeyController.registerRoutes(app);

	LOG_INFO("服务器启动成功，监听端口: 18080");
	app.port(18080).multithreaded().run();
	

	return 0;
}

