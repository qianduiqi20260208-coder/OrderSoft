#include "CustomerInfoController.h"
#include "jwt_utils.h"

// CustomerInfoController::CustomerInfoController(std::shared_ptr<ICustomerInfoService> sp) : customerInfoService(sp) {}
CustomerInfoController::CustomerInfoController() {

}

void CustomerInfoController::registerRoutes(crow::SimpleApp& app) {
    // 获取加密狗列表
    CROW_ROUTE(app, "/dongle/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // // 调用加密狗服务获取加密狗列表
        // auto dongleList = dongleService->getDongleList();
        
        // // 构建返回的JSON数据
        // nlohmann::json dongleArray = nlohmann::json::array();
        
        // for (const auto& dongle : dongleList) {
        //     nlohmann::json dongleItem = {
        //         {"dongleId", dongle.dongleId},
        //         {"shellCode", dongle.shellCode},          // 外壳号
        //         {"shellSerial", dongle.shellSerial},      // 序列号
        //         {"status", dongle.status},                // 状态：available/in_use/returned
        //         {"currentClient", dongle.currentClient},  // 当前所属客户
        //         {"deviceType", dongle.deviceType},        // 设备类型：lab/IPT/FTD/FFS
        //         {"deviceNote", dongle.deviceNote},        // 设备备注
        //         {"deliveryDate", dongle.deliveryDate},    // 交付日期
        //         {"returnDate", dongle.returnDate},        // 归还日期
        //         {"createTime", dongle.createTime},        // 创建时间
        //         {"updateTime", dongle.updateTime}         // 更新时间
        //     };
        //     dongleArray.push_back(dongleItem);
        // }

        // 直接返回mock数据
        nlohmann::json dongleArray = nlohmann::json::array();
        
        // Mock数据1
        dongleArray.push_back({
            {"dongleId", "1"},
            {"shellCode", "A53000000001"},
            {"shellSerial", "89172CAS240919239001890"},
            {"dongleRemark", "CAE技术有限公司专用加密狗"},
            {"dongleStatus", "出库"},
            {"clientName", "CAE"},
            {"deviceType", "lab"},
            {"note", "实验室环境专用设备，用于CAE计算测试"},
            {"inTime", "2024-05-06 10:00:00"},
            {"outTime", "2024-07-15 14:30:00"}
        });
        
        // Mock数据2
        dongleArray.push_back({
            {"dongleId", "2"},
            {"shellCode", "A53000000002"},
            {"shellSerial", "89172CAS240919239001891"},
            {"dongleRemark", "华模科技专用加密狗"},
            {"dongleStatus", "出库"},
            {"clientName", "华模科技"},
            {"deviceType", "IPT"},
            {"note", "集成产品团队专用设备，支持多用户协作"},
            {"inTime", "2024-07-20 09:00:00"},
            {"outTime", "2024-07-25 16:00:00"}
        });
        
        // Mock数据3
        dongleArray.push_back({
            {"dongleId", "3"},
            {"shellCode", "A53000000003"},
            {"shellSerial", "89172CAS240919239001892"},
            {"dongleRemark", "测试环境专用加密狗"},
            {"dongleStatus", "入库"},
            {"clientName", "CAE"},
            {"deviceType", "FTD"},
            {"note", "飞行测试数据处理系统专用设备"},
            {"inTime", "2024-07-25 15:00:00"},
            {"outTime", ""}
        });
        
        // Mock数据4
        dongleArray.push_back({
            {"dongleId", "4"},
            {"shellCode", "A53000000004"},
            {"shellSerial", "89172CAS240919239001893"},
            {"dongleRemark", "新入库加密狗，待分配"},
            {"dongleStatus", "入库"},
            {"clientName", "暂无"},
            {"deviceType", "lab"},
            {"note", "待分配给客户的新设备"},
            {"inTime", "2025-07-28 10:00:00"},
            {"outTime", ""}
        });
        
        // Mock数据5
        dongleArray.push_back({
            {"dongleId", "5"},
            {"shellCode", "A53000000005"},
            {"shellSerial", "89172CAS240919239001894"},
            {"dongleRemark", "损坏设备，需要维修"},
            {"dongleStatus", "损坏"},
            {"clientName", "华模科技"},
            {"deviceType", "FFS"},
            {"note", "全功能仿真系统，硬件故障待维修"},
            {"inTime", "2024-01-15 14:00:00"},
            {"outTime", "2024-06-20 10:30:00"}
        });
        
        // Mock数据6
        dongleArray.push_back({
            {"dongleId", "6"},
            {"shellCode", "A53000000006"},
            {"shellSerial", "89172CAS240919239001895"},
            {"dongleRemark", "丢失设备记录"},
            {"dongleStatus", "丢失"},
            {"clientName", "CAE"},
            {"deviceType", "lab"},
            {"note", "实验室设备，已确认丢失"},
            {"inTime", "2024-03-01 08:00:00"},
            {"outTime", "2024-04-15 17:00:00"}
        });
        
        // Mock数据7
        dongleArray.push_back({
            {"dongleId", "7"},
            {"shellCode", "A53000000007"},
            {"shellSerial", "89172CAS240919239001896"},
            {"dongleRemark", "备用加密狗"},
            {"dongleStatus", "入库"},
            {"clientName", "暂无"},
            {"deviceType", "lab"},
            {"note", "备用设备，待分配"},
            {"inTime", "2025-07-30 11:00:00"},
            {"outTime", ""}
        });
        
        // Mock数据8
        dongleArray.push_back({
            {"dongleId", "8"},
            {"shellCode", "A53000000008"},
            {"shellSerial", "89172CAS240919239001897"},
            {"dongleRemark", "高级授权加密狗"},
            {"dongleStatus", "出库"},
            {"clientName", "CAE"},
            {"deviceType", "IPT"},
            {"note", "高级功能测试设备"},
            {"inTime", "2025-07-01 09:00:00"},
            {"outTime", "2025-07-01 15:00:00"}
        });

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"success", true},
                {"message", "获取加密狗列表成功"},
                {"list", dongleArray},
                {"total", 8}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取特定加密狗的历史记录
    CROW_ROUTE(app, "/dongle/<string>/history").methods("GET"_method)
        ([this](const crow::request& req, const std::string& dongleId) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // // TODO: 调用服务层获取历史记录
        // // auto historyList = dongleService->getDongleHistory(dongleId);

        // nlohmann::json resp = {
        //     {"status", 1},
        //     {"error", ""},
        //     {"data", {
        //         {"list", nlohmann::json::array()},
        //         {"total", 0}
        //     }}
        // };

    // 构建历史记录mock数据
    nlohmann::json historyList = nlohmann::json::array();
    
    if (dongleId == "1") {
        historyList = {
            {
                {"id", "1001"},
                {"outTime", "2024-06-15 14:30:00"},
                {"inTime", "2024-07-25 16:00:00"},
                {"clientName", "CAE技术有限公司"},
                {"authorizations", {
                    {
                        {"generateDate", "2024-06-16"},
                        {"authId", "2025071001"},
                        {"startDate", "2024-06-15"},
                        {"endDate", "2024-12-31"},
                        {"authType", "本地锁"},
                        {"authNote", "CAE计算模块本地锁授权，适用于单机离线计算"}
                    },
                    {
                        {"generateDate", "2024-07-01"},
                        {"authId", "2025071002"},
                        {"startDate", "2024-07-01"},
                        {"endDate", "2025-08-10"},
                        {"authType", "网络锁"},
                        {"authNote", "网络锁临时授权，即将到期"}
                    },
                    {
                        {"generateDate", "2024-07-15"},
                        {"authId", "2025071003"},
                        {"startDate", "2024-07-15"},
                        {"endDate", "2025-07-20"},
                        {"authType", "软锁授权"},
                        {"authNote", "软锁试用版授权，已到期"}
                    }
                }}
            },
            {
                {"id", "1002"},
                {"outTime", "2024-05-06 10:00:00"},
                {"inTime", "2024-06-15 14:30:00"},
                {"clientName", "CAE技术有限公司"},
                {"authorizations", {
                    {
                        {"generateDate", "2024-05-10"},
                        {"authId", "2025071004"},
                        {"startDate", "2024-05-10"},
                        {"endDate", "2024-11-30"},
                        {"authType", "本地锁"},
                        {"authNote", "初期测试本地锁授权"}
                    }
                }}
            },
            {
                {"id", "1003"},
                {"outTime", "未出库"},
                {"inTime", "2024-05-06 10:00:00"},
                {"clientName", ""},
                {"authorizations", nlohmann::json::array()}
            }
        };
    }
    else if (dongleId == "2") {
        historyList = {
            {
                {"id", "2001"},
                {"outTime", "2024-07-25 16:00:00"},
                {"inTime", "至今"},
                {"clientName", "华模科技"},
                {"authorizations", {
                    {
                        {"generateDate", "2024-07-26"},
                        {"authId", "2025071005"},
                        {"startDate", "2024-07-25"},
                        {"endDate", "2026-08-30"},
                        {"authType", "本地锁"},
                        {"authNote", "华模科技专用本地锁授权"}
                    },
                    {
                        {"generateDate", "2024-08-01"},
                        {"authId", "2025071006"},
                        {"startDate", "2024-08-01"},
                        {"endDate", "2025-12-31"},
                        {"authType", "网络锁"},
                        {"authNote", "华模科技网络锁扩展授权"}
                    }
                }}
            },
            {
                {"id", "2002"},
                {"outTime", "未出库"},
                {"inTime", "2024-07-25 16:00:00"},
                {"clientName", ""},
                {"authorizations", nlohmann::json::array()}
            }
        };
    }
    else if (dongleId == "3") {
        historyList = {
            {
                {"id", "3001"},
                {"outTime", "2024-07-25 15:00:00"},
                {"inTime", "至今"},
                {"clientName", "CAE技术有限公司"},
                {"authorizations", {
                    {
                        {"generateDate", "2024-07-26"},
                        {"authId", "2025071007"},
                        {"startDate", "2024-07-25"},
                        {"endDate", "2025-08-03"},
                        {"authType", "网络锁"},
                        {"authNote", "测试环境网络锁，即将过期需续费"}
                    }
                }}
            }
        };
    }
    else if (dongleId == "4") {
        historyList = {
            {
                {"id", "4001"},
                {"outTime", "2025-07-28 10:00:00"},
                {"inTime", "未入库"},
                {"clientName", "暂无"},
                {"authorizations", nlohmann::json::array()}
            }
        };
    }
    else if (dongleId == "5") {
        historyList = {
            {
                {"id", "5001"},
                {"outTime", "2024-06-20 10:30:00"},
                {"inTime", "2024-07-15 14:00:00"},
                {"clientName", "华模科技"},
                {"authorizations", {
                    {
                        {"generateDate", "2024-01-16"},
                        {"authId", "2025071007"},
                        {"startDate", "2024-01-01"},
                        {"endDate", "2024-12-31"},
                        {"authType", "本地锁"},
                        {"authNote", "设备损坏前的授权，已过期"}
                    }
                }}
            },
            {
                {"id", "5002"},
                {"outTime", "未出库"},
                {"inTime", "2024-01-15 14:00:00"},
                {"clientName", ""},
                {"authorizations", nlohmann::json::array()}
            }
        };
    }
    else if (dongleId == "6") {
        historyList = {
            {
                {"id", "6001"},
                {"outTime", "2024-04-15 17:00:00"},
                {"inTime", "丢失"},
                {"clientName", "CAE"},
                {"authorizations", {
                    {
                        {"generateDate", "2024-03-02"},
                        {"authId", "2025071008"},
                        {"startDate", "2024-03-01"},
                        {"endDate", "2025-03-01"},
                        {"authType", "软锁授权"},
                        {"authNote", "丢失设备的软锁授权，已停用"}
                    }
                }}
            },
            {
                {"id", "6002"},
                {"outTime", "未出库"},
                {"inTime", "2024-03-01 08:00:00"},
                {"clientName", ""},
                {"authorizations", nlohmann::json::array()}
            }
        };
    }
    else if (dongleId == "7") {
        historyList = {
            {
                {"id", "7001"},
                {"outTime", "未出库"},
                {"inTime", "2025-07-30 11:00:00"},
                {"clientName", "暂无"},
                {"authorizations", {
                    {
                        {"generateDate", "2025-01-02"},
                        {"authId", "2025071009"},
                        {"startDate", "2025-01-01"},
                        {"endDate", "2025-12-31"},
                        {"authType", "本地锁"},
                        {"authNote", "备用设备本地锁授权"}
                    }
                }}
            }
        };
    }
    else if (dongleId == "8") {
        historyList = {
            {
                {"id", "8001"},
                {"outTime", "2025-07-01 15:00:00"},
                {"inTime", "至今"},
                {"clientName", "CAE"},
                {"authorizations", {
                    {
                        {"generateDate", "2025-07-02"},
                        {"authId", "2025071010"},
                        {"startDate", "2025-07-01"},
                        {"endDate", "2026-07-01"},
                        {"authType", "网络锁"},
                        {"authNote", "高级功能网络锁授权，一年期"}
                    },
                    {
                        {"generateDate", "2025-07-11"},
                        {"authId", "2025071011"},
                        {"startDate", "2025-07-10"},
                        {"endDate", "2025-12-31"},
                        {"authType", "软锁授权"},
                        {"authNote", "扩展模块软锁授权"}
                    }
                }}
            },
            {
                {"id", "8002"},
                {"outTime", "未出库"},
                {"inTime", "2025-07-01 09:00:00"},
                {"clientName", ""},
                {"authorizations", nlohmann::json::array()}
            }
        };
    }

    nlohmann::json resp = {
        {"status", 1},
        {"error", ""},
        {"data", {
            {"dongleId", dongleId},
            {"total", historyList.size()},
            {"list", historyList}
        }}
    };

    return crow::response{ resp.dump() };
        });

    // 获取客户列表信息
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
                    {"status", 0},
                    {"error", "缺少必要参数：clientName"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string clientInfo = reqData.value("clientinfo", ""); // 可选参数

            printf("[DEBUG] 新建客户，clientName: %s, clientInfo: %s\n", 
                   clientName.c_str(), clientInfo.c_str());

            // TODO: 调用服务层创建客户
            // bool result = clientService->createClient(clientName, clientInfo);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "客户创建成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 新建客户失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 0},
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
                    {"status", 0},
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

            // TODO: 调用服务层更新客户信息
            // bool result = clientService->updateClient(originalClientName, clientName, clientInfo);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "客户信息更新成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 编辑客户失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 0},
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
                {"status", 0},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取发送详情信息，clientName: %s\n", clientName.c_str());

        // TODO: 调用服务层获取发送详情
        // auto sendDetailList = clientService->getSendDetail(clientName);

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
                {"status", 0},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取发送详情（分页），clientName: %s, page: %d, pageSize: %d\n", 
               clientName.c_str(), page, pageSize);

        // TODO: 调用服务层获取分页发送详情
        // auto sendDetailList = clientService->getSendDetailWithPagination(clientName, page, pageSize);

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
                {"status", 0},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取发送总览，clientName: %s\n", clientName.c_str());

        // TODO: 调用服务层获取发送总览
        // auto sendOverview = clientService->getSendOverview(clientName);

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
                {"status", 0},
                {"error", "缺少必要参数：clientName"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        printf("[DEBUG] 获取授权详情信息，clientName: %s\n", clientName.c_str());

        // TODO: 调用服务层获取授权详情
        // auto authDetailList = clientService->getAuthDetail(clientName);

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
                {"status", 0},
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

    // 交付外壳
    CROW_ROUTE(app, "/shell/deliver").methods("POST"_method)
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
                    {"status", 0},
                    {"error", "缺少必要参数：clientName、shellNumber、deviceType 或 deviceNote"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string shellNumber = reqData["shellNumber"];
            std::string deviceType = reqData["deviceType"];
            std::string deviceNote = reqData["deviceNote"];

            printf("[DEBUG] 交付外壳，clientName: %s, shellNumber: %s, deviceType: %s, deviceNote: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), deviceType.c_str(), deviceNote.c_str());

            // TODO: 调用服务层交付外壳
            // bool result = shellService->deliverShell(clientName, shellNumber, deviceType, deviceNote);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "外壳交付成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 交付外壳失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 0},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

    // 归还外壳
    CROW_ROUTE(app, "/shell/return").methods("POST"_method)
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
                !reqData.contains("returnDate")) {
                nlohmann::json resp = {
                    {"status", 0},
                    {"error", "缺少必要参数：clientName、shellNumber 或 returnDate"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string shellNumber = reqData["shellNumber"];
            std::string returnDate = reqData["returnDate"];

            printf("[DEBUG] 归还外壳，clientName: %s, shellNumber: %s, returnDate: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), returnDate.c_str());

            // TODO: 调用服务层归还外壳
            // bool result = shellService->returnShell(clientName, shellNumber, returnDate);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "外壳归还成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 归还外壳失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 0},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

    // 新建授权信息
    CROW_ROUTE(app, "/auth/create").methods("POST"_method)
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
                !reqData.contains("authType") || !reqData.contains("startDate") || 
                !reqData.contains("endDate") || !reqData.contains("authNote")) {
                nlohmann::json resp = {
                    {"status", 0},
                    {"error", "缺少必要参数：clientName、shellNumber、authType、startDate、endDate 或 authNote"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string shellNumber = reqData["shellNumber"];
            std::string authId = reqData.value("authId", ""); // 可选参数
            std::string authType = reqData["authType"];
            std::string startDate = reqData["startDate"];
            std::string endDate = reqData["endDate"];
            std::string authNote = reqData["authNote"];

            printf("[DEBUG] 新建授权信息，clientName: %s, shellNumber: %s, authId: %s, authType: %s, startDate: %s, endDate: %s, authNote: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), authId.c_str(), authType.c_str(), 
                   startDate.c_str(), endDate.c_str(), authNote.c_str());

            // TODO: 调用服务层创建授权
            // bool result = authService->createAuth(clientName, shellNumber, authId, authType, startDate, endDate, authNote);

            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "授权信息创建成功"}
                }}
            };
            
            return crow::response{ resp.dump() };

        } catch (const std::exception& e) {
            printf("[ERROR] 新建授权信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 0},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
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
                    {"status", 0},
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
                {"status", 0},
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
                    {"status", 0},
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
                    {"status", 0},
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
                {"status", 0},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

}