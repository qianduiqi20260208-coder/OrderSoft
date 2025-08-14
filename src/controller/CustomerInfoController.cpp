#include "CustomerInfoController.h"
#include <jwt_utils.h>

CustomerInfoController::CustomerInfoController(std::shared_ptr<ICustomerInfoService> service)
    : customerInfoService_(service)
{
}

void CustomerInfoController::registerRoutes(crow::SimpleApp& app) {

    
    // // 注册获取所有客户名称列表路由
    // CROW_ROUTE(app, "/customer-info/clients").methods("GET"_method)
    //     ([this](const crow::request& req) {
    //         return handleGetAllClientNames(req);
    //     });

    // 获取特定加密狗的历史记录
    CROW_ROUTE(app, "/dongle/<string>/history").methods("GET"_method)
        ([this](const crow::request& req, const std::string& dongleId) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // // TODO: 调用服务层获取历史记录
        // // auto historyList = dongleService->getDongleHistory(dongleId);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", nlohmann::json::array()},
                {"total", 0}
            }}
        };

        return crow::response{ resp.dump() };
        });

    // 获取客户列表信息-客户管理页加载
    CROW_ROUTE(app, "/client/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        printf("[DEBUG] 获取客户列表信息\n");

        // TODO: 调用服务层获取客户列表
        // auto clientList = clientService->getClientList();

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", nlohmann::json::array()},
                {"total", 0}
            }}
        };
        
        return crow::response{ resp.dump() };
        });

    // 新建客户
    CROW_ROUTE(app, "/client/create").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体
            nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            if (!reqData.contains("clientName")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：clientName"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string clientInfo = reqData.value("clientinfo", ""); 

            printf("[DEBUG] 新建客户，clientName: %s, clientInfo: %s\n", 
                   clientName.c_str(), clientInfo.c_str());

            // // TODO: 调用服务层创建客户
            // bool result = customerInfoService->addClientInfo(clientName, clientInfo);

            bool result = true;

            if(result)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "客户创建成功"}
                    }}
                };
                
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "客户创建失败"},
                    {"data", {
                        {"success", false},
                        {"message", "客户创建失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }



        } catch (const std::exception& e) {
            printf("[ERROR] 新建客户失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

    // 编辑客户
    CROW_ROUTE(app, "/client/update").methods("PUT"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体
            nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            if (!reqData.contains("originalClientName") || !reqData.contains("clientName")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：originalClientName 或 clientName"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string originalClientName = reqData["originalClientName"];
            std::string clientName = reqData["clientName"];
            std::string clientInfo = reqData.value("clientinfo", ""); // 可选参数

            printf("[DEBUG] 编辑客户，originalClientName: %s, clientName: %s, clientInfo: %s\n", 
                   originalClientName.c_str(), clientName.c_str(), clientInfo.c_str());

            // // TODO: 调用服务层更新客户信息
            // bool result = customerInfoService->alterClientInfo(originalClientName, clientName, clientInfo);

            bool result = true;

            if(result)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "客户信息更新成功"}
                    }}
                };
                
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "客户信息更新失败"},
                    {"data", {
                        {"success", false},
                        {"message", "客户信息更新失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }



        } catch (const std::exception& e) {
            printf("[ERROR] 编辑客户失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

    // 获取发送详情信息
    CROW_ROUTE(app, "/client/send/detail").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 从查询参数中获取clientName
        auto params = crow::query_string(req.url_params);
        std::string clientName = "";
        if (params.get("clientName") != nullptr) {
            clientName = params.get("clientName");
        }

        // 参数验证
        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取发送详情信息，clientName: %s\n", clientName.c_str());

        // TODO: 调用服务层获取发送详情
        // auto sendDetailList = customerInfoService->getSendDetail(clientName);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", nlohmann::json::array()},
                {"total", 0}
            }}
        };
        
        return crow::response{ resp.dump() };
        });

    // 获取发送详情（分页版本）
    CROW_ROUTE(app, "/client/send/detail/pagination").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 从查询参数中获取参数
        auto params = crow::query_string(req.url_params);
        std::string clientName = "";
        int page = 1;
        int pageSize = 10;

        if (params.get("clientName") != nullptr) {
            clientName = params.get("clientName");
        }
        if (params.get("page") != nullptr) {
            page = std::stoi(params.get("page"));
        }
        if (params.get("pageSize") != nullptr) {
            pageSize = std::stoi(params.get("pageSize"));
        }

        // 参数验证
        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取发送详情（分页），clientName: %s, page: %d, pageSize: %d\n", 
               clientName.c_str(), page, pageSize);

        // TODO: 调用服务层获取分页发送详情
        // auto sendDetailList = customerInfoService->getSendDetailWithPagination(clientName, page, pageSize);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", nlohmann::json::array()},
                {"total", 0},
                {"page", page},
                {"pageSize", pageSize}
            }}
        };
        
        return crow::response{ resp.dump() };
        });

    // 获取发送总览（各模型最新版本）
    CROW_ROUTE(app, "/client/send/overview").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 从查询参数中获取clientName
        auto params = crow::query_string(req.url_params);
        std::string clientName = "";
        if (params.get("clientName") != nullptr) {
            clientName = params.get("clientName");
        }

        // 参数验证
        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取发送总览，clientName: %s\n", clientName.c_str());

        // TODO: 调用服务层获取发送总览
        // auto sendOverview = customerInfoService->getSendOverview(clientName);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", nlohmann::json::array()},
                {"total", 0}
            }}
        };
        
        return crow::response{ resp.dump() };
        });

    // 获取授权详情信息
    CROW_ROUTE(app, "/client/auth/detail").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 从查询参数中获取clientName
        auto params = crow::query_string(req.url_params);
        std::string clientName = "";
        if (params.get("clientName") != nullptr) {
            clientName = params.get("clientName");
        }

        // 参数验证
        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取授权详情信息，clientName: %s\n", clientName.c_str());

        // TODO: 调用服务层获取授权详情
        nlohmann::json result = customerInfoService_->getClientAuthInfoJson(clientName);

        // nlohmann::json resp = {
        //     {"status", 1},
        //     {"error", ""},
        //     {"data", {
        //         {"list", nlohmann::json::array()},
        //         {"total", 0}
        //     }}
        // };
        
        return crow::response{ result.dump() };
        });

    // 获取单个外壳号的完整授权列表
    CROW_ROUTE(app, "/shell/auth/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 从查询参数中获取参数
        auto params = crow::query_string(req.url_params);
        std::string clientName = "";
        std::string shellNumber = "";

        if (params.get("clientName") != nullptr) {
            clientName = params.get("clientName");
        }
        if (params.get("shellNumber") != nullptr) {
            shellNumber = params.get("shellNumber");
        }

        // 参数验证
        if (clientName.empty() || shellNumber.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName 或 shellNumber"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取外壳号授权列表，clientName: %s, shellNumber: %s\n", 
               clientName.c_str(), shellNumber.c_str());

        // TODO: 调用服务层获取外壳号授权列表
        // auto shellAuthList = shellService->getShellAuthList(clientName, shellNumber);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", nlohmann::json::array()},
                {"total", 0}
            }}
        };
        
        return crow::response{ resp.dump() };
        });

    // 更新外壳号信息
    CROW_ROUTE(app, "/shell/update").methods("PUT"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体
            nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            if (!reqData.contains("clientName") || !reqData.contains("shellNumber") || 
                !reqData.contains("deviceType") || !reqData.contains("deviceNote")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：clientName、shellNumber、deviceType 或 deviceNote"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string shellNumber = reqData["shellNumber"];
            std::string deviceType = reqData["deviceType"];
            std::string deviceNote = reqData["deviceNote"];

            printf("[DEBUG] 更新外壳号信息，clientName: %s, shellNumber: %s, deviceType: %s, deviceNote: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), deviceType.c_str(), deviceNote.c_str());

            // TODO: 调用服务层更新外壳号信息
            // bool result = shellService->updateShellInfo(clientName, shellNumber, deviceType, deviceNote);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "外壳号信息更新成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 更新外壳号信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

    // 批量更新授权截止日期
    CROW_ROUTE(app, "/auth/batch-update").methods("PUT"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体
            nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            if (!reqData.contains("clientName") || !reqData.contains("changes")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：clientName 或 changes"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            nlohmann::json changes = reqData["changes"];

            // 验证changes数组格式
            if (!changes.is_array()) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "参数格式错误：changes 必须是数组"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            printf("[DEBUG] 批量更新授权截止日期，clientName: %s, 更新数量: %zu\n", 
                   clientName.c_str(), changes.size());

            // 打印每个更新项的详细信息
            for (const auto& change : changes) {
                if (change.contains("shellNumber") && change.contains("authId") && change.contains("newEndDate")) {
                    printf("[DEBUG] 更新项: shellNumber=%s, authId=%s, newEndDate=%s\n",
                           change["shellNumber"].get<std::string>().c_str(),
                           change["authId"].get<std::string>().c_str(),
                           change["newEndDate"].get<std::string>().c_str());
                }
            }

            // TODO: 调用服务层批量更新授权截止日期
            // bool result = authService->batchUpdateAuthEndDates(clientName, changes);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "授权截止日期批量更新成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 批量更新授权截止日期失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

}

std::string CustomerInfoController::getClientAuthInfo(const std::string& clientName)
{
    // 调用Service层获取客户信息和加密狗授权信息，并转换为JSON格式
    nlohmann::json result = customerInfoService_->getClientAuthInfoJson(clientName);
    
    // 将JSON对象转换为字符串返回
    return result.dump();
}

crow::response CustomerInfoController::handleGetClientAuthInfo(const crow::request& req, const std::string& clientName)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    // 参数验证
    if (clientName.empty()) {
        return crow::response(400, R"({"status":0,"error":"客户名称不能为空","data":{}})");
    }
    
    try {
        // 调用业务逻辑获取客户授权信息
        std::string result = getClientAuthInfo(clientName);
        
        // 设置响应头
        crow::response response(200, result);
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    }
    catch (const std::exception& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", std::string("获取客户授权信息失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
}

crow::response CustomerInfoController::handleGetAllClientNames(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    try {
        // 调用业务逻辑获取所有客户名称
        std::string result = getAllClientNames();
        
        // 设置响应头
        crow::response response(200, result);
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    }
    catch (const std::exception& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", std::string("获取客户名称列表失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
}

std::string CustomerInfoController::getAllClientNames()
{
    // 调用Service层获取所有客户名称列表
    std::vector<std::string> clientNames = customerInfoService_->getAllClientNames();
    
    // 构建返回的JSON数据
    nlohmann::json result;
    result["error"] = "";
    result["status"] = 1;
    result["data"] = nlohmann::json::array();
    
    // 将客户名称列表添加到JSON数据中
    for (const auto& clientName : clientNames)
    {
        result["data"].push_back(clientName);
    }
    
    // 将JSON对象转换为字符串返回
    return result.dump();
}