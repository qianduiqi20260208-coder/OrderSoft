#include "CustomerInfoController.h"
#include <jwt_utils.h>
#include "Log.h"
#include "Logger.h"

CustomerInfoController::CustomerInfoController(std::shared_ptr<ICustomerInfoService> service)
    : customerInfoService_(service)
{
}

void CustomerInfoController::registerRoutes(crow::App<crow::CORSHandler>& app) {

    // 获取客户列表信息-客户管理页加载
    CROW_ROUTE(app, "/client/list").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

         LOG_DEBUG("获取客户列表信息\n");

        // 调用服务层获取客户列表
        nlohmann::json clientListResult = customerInfoService_->getClientList();
        
        // 如果获取成功，重新格式化数据结构以匹配前端期望的格式
        if (clientListResult["status"] == 1) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"list", clientListResult["data"]},
                    {"total", clientListResult["data"].size()}
                }}
            };
            return crow::response{ resp.dump() };
        } else {
            // 如果获取失败，返回错误信息
            nlohmann::json resp = {
                {"status", 0},
                {"error", clientListResult["error"]},
                {"data", {
                    {"list", nlohmann::json::array()},
                    {"total", 0}
                }}
            };
            return crow::response{ resp.dump() };
         }
        }));

    // 新建客户
    CROW_ROUTE(app, "/client/create").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
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

             LOG_DEBUG("新建客户，clientName: %s, clientInfo: %s\n", 
                   clientName.c_str(), clientInfo.c_str());

            // TODO: 调用服务层创建客户
            bool result = customerInfoService_->addClientInfo(clientName, clientInfo);

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
            LOG_ERROR("新建客户失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));

    // 客户名称迁移（跨表重命名，处理外键）
    CROW_ROUTE(app, "/client/migrate").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
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
            std::string newClientName = reqData["clientName"];
            std::string newClientInfo = reqData.value("clientinfo", "");

            LOG_DEBUG("迁移客户名称，originalClientName: %s, newClientName: %s, newClientInfo: %s\n", 
                   originalClientName.c_str(), newClientName.c_str(), newClientInfo.c_str());

            bool result = customerInfoService_->migrateCustomerName(originalClientName, newClientName, newClientInfo);

            if (result) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "客户名称迁移成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            } else {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "客户名称迁移失败"},
                    {"data", {
                        {"success", false},
                        {"message", "客户名称迁移失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
        } catch (const std::exception& e) {
            LOG_ERROR("客户名称迁移失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));

    // 获取发送详情信息
    CROW_ROUTE(app, "/client/send/detail").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
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

         LOG_DEBUG("获取发送详情信息，clientName: %s\n", clientName.c_str());

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
        }));

    // 获取客户模型版本历史信息
    CROW_ROUTE(app, "/client/model-version-history").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        // if (!checkToken(req)) {
        //     return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        // }

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

        LOG_DEBUG("获取客户模型版本历史信息，clientName: %s\n", clientName.c_str());

        try {
            // 调用Service层获取客户模型版本历史信息
            nlohmann::json result = customerInfoService_->getCustomerModelVersionHistory(clientName);
            
            // 设置响应头
            crow::response response(200, result.dump());
            response.add_header("Content-Type", "application/json; charset=utf-8");
            return response;
        }
        catch (const std::exception& e) {
            nlohmann::json errorResponse = {
                {"status", 0},
                {"error", std::string("获取客户模型版本历史失败: ") + e.what()},
                {"data", nlohmann::json::object()}
            };
            return crow::response(500, errorResponse.dump());
        }
        }));

    // 获取发送详情（分页版本）
    CROW_ROUTE(app, "/client/send/detail/pagination").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 获取参数
        auto params = crow::query_string(req.url_params);
        std::string clientName = params.get("clientName") ? params.get("clientName") : "";
        int page = params.get("page") ? std::stoi(params.get("page")) : 1;
        int pageSize = params.get("pageSize") ? std::stoi(params.get("pageSize")) : 10;

        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        // 获取分页数据
        auto pagedSendDetail = customerInfoService_->getSendRecordByClientPagedByDate(clientName, page - 1, pageSize);

        // 统计模型和版本数量
        std::pair<int, int> modelStats = customerInfoService_->getModelAndModelVersionCountByClient(clientName);


        nlohmann::json dailySendRecords = nlohmann::json::array();
        for (const auto& groupPair : pagedSendDetail) {
            const auto& overview = groupPair.first; // SendOverview: [date, sendCount]
            const auto& records = groupPair.second; // vector<SendRecord>: [modelId, modelVersionId, orderId, sendTime]

            nlohmann::json recordArray = nlohmann::json::array();
            for (const auto& record : records) {
                recordArray.push_back({
                    {"modelId",        record.size() > 0 ? record[0] : ""},
                    {"modelVersionId", record.size() > 1 ? record[1] : ""},
                    {"orderId",        record.size() > 2 ? record[2] : ""},
                    {"sendTime",       record.size() > 3 ? record[3] : ""}
                });
            }

            dailySendRecords.push_back({
                {"date",      overview.size() > 0 ? overview[0] : ""},
                {"sendCount", overview.size() > 1 ? std::stoi(overview[1]) : static_cast<int>(records.size())},
                {"records",   recordArray}
            });
        }

        // 总数建议由服务层返回或单独统计
        int total = customerInfoService_->getAllSendRecordGroupedCountByClient(clientName);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"clientName", clientName},
                {"dailySendRecords", dailySendRecords},
                {"total", total},
                {"modelCount", modelStats.first},
                {"modelVersionCount", modelStats.second},
                {"page", page},
                {"pageSize", pageSize}
            }}
        };

        return crow::response{ resp.dump() };
    }));

    // 获取发送总览（各模型最新版本）
    CROW_ROUTE(app, "/client/send/overview").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 获取参数
        auto params = crow::query_string(req.url_params);
        std::string clientName = params.get("clientName") ? params.get("clientName") : "";

        // 参数验证
        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：clientName"},
                {"data", {{"success", false}, {"latestVersionRecords", nlohmann::json::array()}}}
            };
            return crow::response(400, resp.dump());
        }

         LOG_DEBUG("获取发送总览，clientName: %s\n", clientName.c_str());

        // 获取各模型最新版本发送记录
        auto sendOverview = customerInfoService_->getAllModelLatestVesrionByClient(clientName);

        nlohmann::json latestVersionRecords = nlohmann::json::array();
        for (const auto& record : sendOverview) {
            latestVersionRecords.push_back({
                {"modelId",        record.size() > 0 ? record[0] : ""},
                {"orderId",        record.size() > 1 ? record[1] : ""},
                {"modelVersionId", record.size() > 2 ? record[2] : ""},
                {"sendTime",       record.size() > 3 ? record[3] : ""}
            });
        }

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"success", true},
                {"latestVersionRecords", latestVersionRecords}
            }}
        };

        return crow::response{ resp.dump() };
    }));

    // 获取授权详情信息
    CROW_ROUTE(app, "/client/auth/detail").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
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

         LOG_DEBUG("获取授权详情信息，clientName: %s\n", clientName.c_str());

        // 调用服务层获取授权详情
        nlohmann::json result = customerInfoService_->getClientAuthInfoJson(clientName);
        return crow::response{ result.dump() };
        }));

    // 获取单个外壳号的完整授权列表
    CROW_ROUTE(app, "/shell/auth/list").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
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

         LOG_DEBUG("获取外壳号授权列表，clientName: %s, shellNumber: %s\n", 
               clientName.c_str(), shellNumber.c_str());

        // 调用服务层获取外壳号授权列表
        nlohmann::json resp = customerInfoService_->getShellAuthorizationInfoJson(clientName, shellNumber);
        
        return crow::response{ resp.dump() };
        }));

    // 获取外壳号列表-填写发送工单使用
    CROW_ROUTE(app, "/order/shell-numbers").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 从查询参数中获取targetCustomer
        std::string targetCustomer = "";
        auto params = crow::query_string(req.url_params);
        if (params.get("targetCustomer") != nullptr) {
            targetCustomer = params.get("targetCustomer");
        }

        // 参数验证
        if (targetCustomer.empty()) {
            nlohmann::json resp = {
                {"status", 0},
                {"error", "缺少必要参数：targetCustomer"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        std::vector<std::string> shellNumber_ = customerInfoService_->getEncryptionKeyByClient(targetCustomer);
        nlohmann::json shellNumberJson = nlohmann::json::array();
        for (const auto& sn : shellNumber_) {
            shellNumberJson.push_back(sn);
        }

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", shellNumberJson}
            }}
        };
        return crow::response{ resp.dump() };
        }));

    // 获取授权ID列表-填写发送工单使用
    CROW_ROUTE(app, "/order/auth-ids").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 从查询参数中获取参数
        auto params = crow::query_string(req.url_params);
        std::string targetCustomer = "";
        std::string shellNumber = "";

        if (params.get("targetCustomer") != nullptr) {
            targetCustomer = params.get("targetCustomer");
        }
        if (params.get("shellNumber") != nullptr) {
            shellNumber = params.get("shellNumber");
        }

        // 参数验证
        if (targetCustomer.empty() || shellNumber.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：targetCustomer 或 shellNumber"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

         LOG_DEBUG("获取外壳号授权列表，targetCustomer: %s, shellNumber: %s\n", 
               targetCustomer.c_str(), shellNumber.c_str());
        
        nlohmann::json authIDList = nlohmann::json::array();
        std::vector<Authorization> authList = customerInfoService_->getShellAuthorization(targetCustomer, shellNumber);
        for (const auto& auth : authList) {
            nlohmann::json authItem;
            authItem["authId"] = auth.authId;
            authItem["endDate"] = auth.endDate;
            authItem["deviceType"] = auth.authType;
            authItem["description"] = auth.authNote;
            authIDList.push_back(authItem);
        }
        
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", authIDList}
            }}
        };
        return crow::response{ resp.dump() };
        }));

    // 获取客户的所有授权ID列表
    // CROW_ROUTE(app, "/order/customer-auth-ids").methods("GET"_method)
    //     (withAspect([this](const crow::request& req) {
    //     if (!checkToken(req)) {
    //         return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    //     }
    //     // 从查询参数中获取参数
    //     auto params = crow::query_string(req.url_params);
    //     std::string targetCustomer = "";

    //     if (params.get("targetCustomer") != nullptr) {
    //         targetCustomer = params.get("targetCustomer");
    //     }

    //     // 参数验证
    //     if (targetCustomer.empty()) {
    //         nlohmann::json resp = {
    //             {"status", 1},
    //             {"error", "缺少必要参数：targetCustomer"},
    //             {"data", {}}
    //         };
    //         return crow::response(400, resp.dump());
    //     }

    //      LOG_DEBUG("获取客户所有授权ID列表，targetCustomer: %s\n", 
    //            targetCustomer.c_str());
        
    //     nlohmann::json authIDList = nlohmann::json::array();
    //     std::vector<Authorization> authList = customerInfoService_->getCustomerAllAuthorizations(targetCustomer);
    //     for (const auto& auth : authList) {
    //         nlohmann::json authItem;
    //         authItem["authId"] = auth.authId;
    //         authItem["endDate"] = auth.endDate;
    //         authItem["deviceType"] = auth.authType;
    //         authItem["description"] = auth.authNote;
    //         authIDList.push_back(authItem);
    //     }
        
    //     nlohmann::json resp = {
    //         {"status", 1},
    //         {"error", ""},
    //         {"data", {
    //             {"list", authIDList}
    //         }}
    //     };
    //     return crow::response{ resp.dump() };
    //     }));

    // 根据授权ID获取对应的外壳号
    CROW_ROUTE(app, "/order/shell-by-auth").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 从查询参数中获取参数
        auto params = crow::query_string(req.url_params);
        std::string authId = "";
        std::string targetCustomer = "";

        if (params.get("authId") != nullptr) {
            authId = params.get("authId");
        }
        if (params.get("targetCustomer") != nullptr) {
            targetCustomer = params.get("targetCustomer");
        }

        // 参数验证
        if (authId.empty() || targetCustomer.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：authId 或 targetCustomer"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

         LOG_DEBUG("根据授权ID获取外壳号，authId: %s, targetCustomer: %s\n", 
               authId.c_str(), targetCustomer.c_str());
        
        std::string shellNumber = customerInfoService_->getShellByAuthId(authId, targetCustomer);
        
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"shellNumber", shellNumber}
            }}
        };
        return crow::response{ resp.dump() };
        }));

    // 根据授权ID获取对应的外壳号列表
    CROW_ROUTE(app, "/order/shells-by-auth").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 从查询参数中获取参数
        auto params = crow::query_string(req.url_params);
        std::string authId = "";
        std::string targetCustomer = "";

        if (params.get("authId") != nullptr) {
            authId = params.get("authId");
        }
        if (params.get("targetCustomer") != nullptr) {
            targetCustomer = params.get("targetCustomer");
        }

        // 参数验证
        if (authId.empty() || targetCustomer.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：authId 或 targetCustomer"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

         LOG_DEBUG("根据授权ID获取外壳号列表，authId: %s, targetCustomer: %s\n", 
               authId.c_str(), targetCustomer.c_str());
        
        std::vector<std::string> shellNumbers = customerInfoService_->getShellListByAuthId(authId, targetCustomer);
        
        nlohmann::json shellList = nlohmann::json::array();
        for (const auto& shell : shellNumbers) {
            shellList.push_back(shell);
        }
        
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"shellNumbers", shellList}
            }}
        };
        return crow::response{ resp.dump() };
        }));

    // 获取客户的授权信息（按授权ID分组）
    CROW_ROUTE(app, "/order/customer-auth-ids").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // if (!checkToken(req)) {
        //     return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        // }
        // 从查询参数中获取参数
        auto params = crow::query_string(req.url_params);
        std::string clientName = "";

        if (params.get("clientName") != nullptr) {
            clientName = params.get("clientName");
        }

        // 参数验证
        if (clientName.empty()) {
            nlohmann::json resp = {
                {"status", 0},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

         LOG_DEBUG("获取客户授权信息（按授权ID分组），clientName: %s\n", 
               clientName.c_str());
        
        nlohmann::json result = customerInfoService_->getCustomerAuthorizationsByGroup(clientName);
        
        return crow::response{ result.dump() };
        }));

    // 获取所有客户suffix列表
    CROW_ROUTE(app, "/client/suffixes").methods("GET"_method)
        ([this](const crow::request& req) {
            return handleGetAllClientSuffixes(req);
        });

    // // 注册获取所有客户名称列表路由
    // CROW_ROUTE(app, "/customer/list").methods("GET"_method)
    //     ([this](const crow::request& req) {
    //         // JWT校验
    //         if (!checkToken(req)) {
    //             return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    //         }
            
    //         try {
    //             // 调用业务逻辑获取所有客户名称
    //             std::string result = getAllClientNames();
                
    //             // 设置响应头
    //             crow::response response(200, result);
    //             response.add_header("Content-Type", "application/json; charset=utf-8");
    //             return response;
    //         }
    //         catch (const std::exception& e) {
    //             nlohmann::json errorResponse = {
    //                 {"status", 0},
    //                 {"error", std::string("获取客户名称列表失败: ") + e.what()},
    //                 {"data", nlohmann::json::object()}
    //             };
    //             return crow::response(500, errorResponse.dump());
    //         }
    //     });

}

std::string CustomerInfoController::getClientAuthInfo(const std::string& clientName)
{
    // 调用Service层获取客户信息和加密锁授权信息，并转换为JSON格式
    nlohmann::json result = customerInfoService_->getClientAuthInfoJson(clientName);
    
    // 将JSON对象转换为字符串返回
    return result.dump();
}

crow::response CustomerInfoController::handleGetShellAuthorizationInfo(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    try {
        // 解析JSON请求体
        nlohmann::json requestJson = nlohmann::json::parse(req.body);
        
        // 参数验证
        if (!requestJson.contains("clientName") || !requestJson.contains("shellNumber")) {
            return crow::response(400, R"({"status":0,"error":"缺少必要参数clientName或shellNumber","data":{}})");
        }
        
        std::string clientName = requestJson["clientName"];
        std::string shellNumber = requestJson["shellNumber"];
        
        if (clientName.empty() || shellNumber.empty()) {
            return crow::response(400, R"({"status":0,"error":"客户名称和外壳号不能为空","data":{}})");
        }
        
        // 调用Service层获取指定外壳号的授权信息
        nlohmann::json result = customerInfoService_->getShellAuthorizationInfoJson(clientName, shellNumber);
        
        // 设置响应头
        crow::response response(200, result.dump());
        response.add_header("Content-Type", "application/json; charset=utf-8");
        return response;
    }
    catch (const nlohmann::json::parse_error& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", "JSON解析失败: " + std::string(e.what())},
            {"data", nlohmann::json::object()}
        };
        return crow::response(400, errorResponse.dump());
    }
    catch (const std::exception& e) {
        nlohmann::json errorResponse = {
            {"status", 0},
            {"error", std::string("获取外壳号授权信息失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
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

crow::response CustomerInfoController::handleGetAllClientSuffixes(const crow::request& req)
{
    // JWT校验
    if (!checkToken(req)) {
        return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    }
    
    try {
        LOG_DEBUG("获取所有客户suffix列表\n");
        
        // 调用Service层获取所有客户suffix列表
        std::vector<std::pair<std::string, std::string>> suffixList = customerInfoService_->getAllClientSuffixList();
        
        // 构建JSON数组，包含客户名和后缀信息
        nlohmann::json suffixArray = nlohmann::json::array();
        for (const auto& clientSuffix : suffixList) {
            nlohmann::json suffixItem = {
                {"clientName", clientSuffix.first},
                {"suffix", clientSuffix.second}
            };
            suffixArray.push_back(suffixItem);
        }
        
        // 构建JSON响应
        nlohmann::json response = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"suffixes", suffixArray}
            }}
        };
        
        // 设置响应头
        crow::response res(200, response.dump());
        res.add_header("Content-Type", "application/json; charset=utf-8");
        return res;
    }
    catch (const std::exception& e) {
        LOG_ERROR("获取客户suffix列表失败: %s", e.what());
        nlohmann::json errorResponse = {
            {"status", 1},
            {"error", std::string("获取客户suffix列表失败: ") + e.what()},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResponse.dump());
    }
}