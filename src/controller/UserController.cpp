#include "UserController.h"
#include "jwt_utils.h"

UserController::UserController(std::shared_ptr<IUserService> sp) : userService(sp) {}

// 角色转换函数
std::string UserController::convertRoleToEnglish(const std::string& chineseRole) const {
    if (chineseRole == "模型工程师") {
        return "ModelEngineer";
    } else if (chineseRole == "软件工程师") {
        return "SoftwareEngineer";
    } else if (chineseRole == "访客") {
        return "Guest";
    } else if (chineseRole == "SuperUser") {
        return "SuperUser";
    }
    return "Guest"; // 默认返回访客
}

void UserController::registerRoutes(crow::SimpleApp& app) {
	// 用户登录
    CROW_ROUTE(app, "/user/login").methods("POST"_method)
        ([this](const crow::request& req) {
        // 解析请求体为 JSON
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        // JSON 解析失败，返回 400 错误
        if (body.is_discarded()) {
            crow::response r(400);
            r.set_header("Access-Control-Allow-Origin", "*");
            r.set_header("Content-Type", "application/json");
            r.write(R"({"error":"Invalid JSON","status":0,"data":{}})");
            return r;
        }

        // 获取请求参数
        std::string account = body.value("account", ""); // 用户名
        std::string password = body.value("password", ""); // 密码
        bool remember = body.value("remember", false); // 是否记住密码

        // 调用用户服务进行登录验证
        auto userOpt = userService->login(account, password);

        user = userService->getUserByJobNumber(std::stoi(account));

        crow::response r;
        r.set_header("Access-Control-Allow-Origin", "*");
        r.set_header("Content-Type", "application/json; charset = utf - 8");
        if (userOpt) {
            // 生成JWT token
            std::string token = generateToken(account);

            nlohmann::json j;
            j["account"] = std::to_string(user.jobNumber); // 返回工号 
            j["token"] = token; // 返回token
            j["role"] = "SuperUser"; // 返回用户角色
            j["models"] = user.responsibleModel; // 返回用户关联的模型

            // 登录成功，返回用户信息
            nlohmann::json resp = {
                {"error", ""},
                {"status", 1},
                {"data", j}
            };
            r.code = 200;
            r.write(resp.dump());
        }
        else {
            // 登录失败，返回错误信息
            nlohmann::json resp = {
                {"error", "账号或密码错误"},
                {"status", 0},
                {"data", nlohmann::json::object()}
            };
            r.code = 200;
            r.write(resp.dump());
        }
        return r;
        });

    // 权限
    CROW_ROUTE(app, "/user/permission").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        std::vector<std::string> permissions;

        // 获取用户角色权限
        for(const auto& role : user.roleVec) {
            printf("[info] function:convertRoleToEnglish() role: %s\n", role.c_str());
            std::string role_ = convertRoleToEnglish(role); // 转换为英文角色名称
            permissions.push_back(role_);
        }

        // 获取用户关联的模型
        for (const auto& model : user.responsibleModel) {
            permissions.push_back(model + ".browse");
        }

        // 构建响应
        nlohmann::json resp = {
            {"error", ""},
            {"status", 1},
            {"data", {{"permissions", permissions}}}
        };

        return crow::response{ resp.dump() };
        });

    // 修改密码
    CROW_ROUTE(app, "/user/password/edit").methods("POST"_method)
        ([this](const crow::request& req) {
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"error":"Invalid JSON","status":0,"data":{}})");
        }
        // 实际项目应校验token和原密码，这里直接返回成功
        return crow::response{ nlohmann::json{
            {"error", ""},
            {"status", 1},
            {"data", {{"isSuccess", true}}}
        }.dump() };
            });

    // 获取当前用户工单列表
    CROW_ROUTE(app, "/order/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
		// 解析查询参数，获取用户ID和角色
        auto params = crow::query_string(req.url_params);
        std::string userID = params.get("userID") ? params.get("userID") : "";
        std::string role = params.get("role") ? params.get("role") : "";

        auto tickets = userService->getUserOrder(std::stoi(userID));

        printf("[info] function:getUserOrder() 查询用户工单列表成功！ tickets.size(): %zu\n", tickets.size());

        nlohmann::json list = nlohmann::json::array();
        for (const auto& ticketPtr : tickets) {
            if (ticketPtr) {
                // 多态调用 to_json()
                list.push_back(ticketPtr->to_json());
            }
        }

        // 构建响应
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", list},
                {"total", tickets.size()},
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取审批人信息列表
    CROW_ROUTE(app, "/approver/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        std::map<std::string, std::vector<int>> retMap = userService->getOrderRole();
        nlohmann::json approvers = retMap["审批人"];

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", approvers}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取分发人信息列表
    CROW_ROUTE(app, "/distributor/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        std::map<std::string, std::vector<int>> retMap = userService->getOrderRole();
        nlohmann::json distributors = retMap["分发人"];

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", distributors}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取执行人信息列表
    CROW_ROUTE(app, "/executor/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        std::map<std::string, std::vector<int>> retMap = userService->getOrderRole();
        nlohmann::json executors = retMap["执行人"];

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", executors}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取流转人信息列表
    CROW_ROUTE(app, "/transferExecutor/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        std::map<std::string, std::vector<int>> retMap = userService->getOrderRole();
        nlohmann::json transferExecutors = retMap["执行人"];
        
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", transferExecutors}
            }}
        };
        return crow::response{ resp.dump() };
        });
}