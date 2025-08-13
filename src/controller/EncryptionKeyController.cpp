#include "EncryptionKeyController.h"
#include "jwt_utils.h"

EncryptionKeyController::EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> sp) : encryptionKeyService(sp) {}

void EncryptionKeyController::registerRoutes(crow::SimpleApp& app) {
    // 获取加密狗列表
    CROW_ROUTE(app, "/dongle/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 调用加密狗服务获取加密狗列表
        std::vector<DongleInfo> dongleList = encryptionKeyService->getDongleInfo();

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
            bool result = encryptionKeyService->createEncryptionKey(shellCode, shellSerial);
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
            bool result = encryptionKeyService->updateEncryptionKey(dongleIdInt, shellCode, shellSerial);
            
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

}