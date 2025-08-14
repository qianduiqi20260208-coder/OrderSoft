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

            // TODO: 调用服务层创建加密狗
            bool result = encryptionKeyService_->createEncryptionKey(shellCode, shellSerial);
            if(result)
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
            bool result = encryptionKeyService_->updateEncryptionKey(dongleIdInt, shellCode, shellSerial);
            
            if(result)
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

            // TODO: 调用服务层交付外壳
            bool result = encryptionKeyService_->deliveryOperation(clientName, shellNumber, deviceType, deviceNote);

            if(result)
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
                !reqData.contains("returnDate")) {
                nlohmann::json resp = {
                    {"status", 1},
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
            bool result = encryptionKeyService_->returnOperation(clientName, shellNumber, returnDate);

            if(result)
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

            // TODO: 调用服务层创建授权
            bool result = encryptionKeyService_->createAuthorization(clientName, shellNumber, authId, authType, startDate, endDate, authNote);

            if(result)
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
}