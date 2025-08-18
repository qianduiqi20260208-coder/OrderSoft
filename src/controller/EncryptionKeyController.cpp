#include "EncryptionKeyController.h"
#include <nlohmann/json.hpp>
#include <jwt_utils.h>

EncryptionKeyController::EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> service)
    : encryptionKeyService_(service)
{
}

void EncryptionKeyController::registerRoutes(crow::SimpleApp& app) {
    // 获取加密狗列表
    CROW_ROUTE(app, "/dongle/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 调用加密狗服务获取加密狗列表
        std::vector<DongleInfo> dongleList = encryptionKeyService_->getDongleInfo();

        // 构建返回的JSON数据
        nlohmann::json dongleArray = nlohmann::json::array();

        for(auto dongle : dongleList)
        {
            dongleArray.push_back(dongle.to_json());
        }
        
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"success", true},
                {"message", "获取加密狗列表成功"},
                {"list", dongleArray},
                {"total", dongleArray.size()}
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

        // 获取历史记录
        auto historyList = encryptionKeyService_->getEncryptionKeyHistoryWithAuthRecordByEK(dongleId);

        nlohmann::json historyArray = nlohmann::json::array();
        for (const auto& historyPair : historyList) {
            const auto& history = historyPair.first;   // EncryptionKeyHistory: [id, outTime, inTime, clientName]
            const auto& authList = historyPair.second; // vector<AuthInfo>: [generateDate, authId, startDate, endDate, authType, authNote]

            nlohmann::json authArray = nlohmann::json::array();
            for (const auto& auth : authList) {
                // 防止越界，建议加长度判断
                authArray.push_back({
                    {"generateDate", auth.size() > 0 ? auth[0] : ""},
                    {"authId",       auth.size() > 1 ? auth[1] : ""},
                    {"startDate",    auth.size() > 2 ? auth[2] : ""},
                    {"endDate",      auth.size() > 3 ? auth[3] : ""},
                    {"authType",     auth.size() > 4 ? auth[4] : ""},
                    {"authNote",     auth.size() > 5 ? auth[5] : ""}
                });
            }

            historyArray.push_back({
                {"id",         history.size() > 0 ? history[0] : ""},
                {"outTime",    history.size() > 1 ? history[1] : "未知"},
                {"inTime",     history.size() > 2 ? history[2] : "未知"},
                {"clientName", history.size() > 3 ? history[3] : "暂无客户"},
                {"authorizations", authArray}
            });
        }

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", historyArray},
                {"total", historyArray.size()}
            }}
        };

        return crow::response{ resp.dump() };
    });

    // 创建加密狗
    CROW_ROUTE(app, "/dongle/create").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体
            nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            if (!reqData.contains("shellCode") || !reqData.contains("shellSerial")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：shellCode 或 shellSerial"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string shellCode = reqData["shellCode"];
            std::string shellSerial = reqData["shellSerial"];

            printf("[DEBUG] 创建加密狗，shellCode: %s, shellSerial: %s\n", 
                   shellCode.c_str(), shellSerial.c_str());

            // 调用服务层创建加密狗
            bool success = encryptionKeyService_->createEncryptionKey(shellCode, shellSerial);
            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "加密狗创建成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else{
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "加密狗创建失败"},
                    {"data", {
                        {"success", false},
                        {"message", "加密狗创建失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

        } catch (const std::exception& e) {
            printf("[ERROR] 创建加密狗失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });

    // 更新加密狗信息
    CROW_ROUTE(app, "/delivery/dongles/update").methods("PUT"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体
            nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            if (!reqData.contains("dongleId") || !reqData.contains("shellCode") || !reqData.contains("shellSerial")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：dongleId、shellCode 或 shellSerial"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string dongleId = reqData["dongleId"];
            std::string shellCode = reqData["shellCode"];
            std::string shellSerial = reqData["shellSerial"];

            int dongleIdInt;

            if(dongleId != "")
            {
                dongleIdInt = std::stoi(dongleId);
            }

            printf("[DEBUG] 更新加密狗信息，dongleId: %d, shellCode: %s, shellSerial: %s\n", 
                   dongleIdInt, shellCode.c_str(), shellSerial.c_str());

            // 调用服务层更新加密狗信息
            bool success = encryptionKeyService_->updateEncryptionKey(dongleIdInt, shellCode, shellSerial);
            
            if(success)
            {
                nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "加密狗信息更新成功"}
                }}
            };
            
            return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "加密狗信息更新失败"},
                    {"data", {
                        {"success", false},
                        {"message", "加密狗信息更新失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }


        } catch (const std::exception& e) {
            printf("[ERROR] 更新加密狗信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
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

            printf("[DEBUG] 交付外壳，clientName: %s, shellNumber: %s, deviceType: %s, deviceNote: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), deviceType.c_str(), deviceNote.c_str());

            // 调用服务层交付外壳
            bool success = encryptionKeyService_->deliveryOperation(clientName, shellNumber, deviceType, deviceNote);

            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "外壳交付成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "外壳交付失败"},
                    {"data", {
                        {"success", false},
                        {"message", "外壳交付失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

        } catch (const std::exception& e) {
            printf("[ERROR] 交付外壳失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
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
                !reqData.contains("operationType") || 
                !reqData.contains("inTime") || 
                !reqData.contains("outTime")) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "缺少必要参数：clientName、shellNumber 或 operationType 或 inTime 或 outTime"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            std::string clientName = reqData["clientName"];
            std::string shellNumber = reqData["shellNumber"];
            std::string inTime = reqData["inTime"];
            std::string outTime = reqData["outTime"];
            std::string operationType = reqData["operationType"];
            std::string remark = reqData.contains("remark") ? reqData["remark"] : "";


            printf("[DEBUG] 归还外壳，clientName: %s, shellNumber: %s, inTime: %s, outTime: %s, operationType: %s, remark: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), inTime.c_str(), outTime.c_str(), operationType.c_str(), remark.c_str());

            // 调用服务层归还外壳
            bool success = encryptionKeyService_->returnOperation(clientName, shellNumber, operationType, inTime, outTime, remark);


            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "外壳归还成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "外壳归还失败"},
                    {"data", {
                        {"success", false},
                        {"message", "外壳归还失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

        } catch (const std::exception& e) {
            printf("[ERROR] 归还外壳失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
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
                    {"status", 1},
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

            // 调用服务层创建授权
            bool success = encryptionKeyService_->createAuthorization(clientName, shellNumber, authId, authType, startDate, endDate, authNote);

            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "授权信息创建成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "授权信息创建失败"},
                    {"data", {
                        {"success", false},
                        {"message", "授权信息创建失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

        } catch (const std::exception& e) {
            printf("[ERROR] 新建授权信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        });
        
    // 获取可交付的外壳号列表
    CROW_ROUTE(app, "/delivery/available-shells").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        
        // 调用Service层获取可交付外壳号列表
        auto availableShells = encryptionKeyService_->getAvailableShellNumbers();
        
        // 构建响应数据
        nlohmann::json shellList = nlohmann::json::array();
        for (const auto& shell : availableShells) {
            shellList.push_back({
                {"id", shell.first},
                {"shellNumber", shell.second}
            });
        }
        
        // 构建响应
        nlohmann::json response = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"shells", shellList},
                {"count", availableShells.size()}
            }}
        };
        
        return crow::response{response.dump()};
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

            // 调用服务层更新外壳号信息
            bool success = encryptionKeyService_->updateShellDeviceInfo(clientName, shellNumber, deviceType, deviceNote);

            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "外壳号信息更新成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "外壳号信息更新失败"},
                    {"data", {
                        {"success", false},
                        {"message", "外壳号信息更新失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

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
            nlohmann::json changesJson = reqData["changes"];

            // 验证changes数组格式
            if (!changesJson.is_array()) {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "参数格式错误：changes 必须是数组"},
                    {"data", {}}
                };
                return crow::response(400, resp.dump());
            }

            // 验证changes是数组
            if (!changesJson.is_array()) {
                return crow::response(400, R"({"status":1,"error":"changes参数必须是数组","data":{}})");
            }
            
            // 解析changes数组
            std::vector<std::tuple<std::string, std::string, std::string>> changes;
            for (const auto& change : changesJson) {
                if (!change.contains("shellNumber") || !change.contains("authId") || !change.contains("newEndDate")) {
                    return crow::response(400, R"({"status":0,"error":"changes数组中的每个元素必须包含shellNumber、authId和newEndDate","data":{}})");
                }
                
                std::string shellNumber = change["shellNumber"];
                std::string authId = change["authId"];
                std::string newEndDate = change["newEndDate"];
                
                changes.emplace_back(shellNumber, authId, newEndDate);
            }

            printf("[DEBUG] 批量更新授权截止日期，clientName: %s, 更新数量: %zu\n", 
                   clientName.c_str(), changes.size());


            // 调用服务层批量更新授权截止日期
            bool success = encryptionKeyService_->updateAuthorizationEndDates(clientName, changes);

            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "授权截止日期批量更新成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "授权截止日期批量更新失败"},
                    {"data", {
                        {"success", false},
                        {"message", "授权截止日期批量更新失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

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