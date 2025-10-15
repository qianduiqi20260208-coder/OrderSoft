#include "EncryptionKeyController.h"
#include <nlohmann/json.hpp>
#include <jwt_utils.h>
#include "IniReader.h"
#include "Log.h"
#include "Logger.h"
#include <string>
#include <cstdio>

EncryptionKeyController::EncryptionKeyController(std::shared_ptr<IEncryptionKeyService> service)
    : encryptionKeyService_(service)
{
}

// 解析 multipart/form-data 格式的表单数据
MultipartResult EncryptionKeyController::parseMultipartForm(const std::string& content_type, const std::string& body,TicketReproduce& ticketreproduce)
{
    MultipartResult result; // 用于保存解析结果，包括表单字段和已保存的文件路径

    // 获取 boundary（分隔符），用于分割每个表单部分
    size_t pos_boundary = content_type.find("boundary=");
    std::string boundary = (pos_boundary != std::string::npos)
        ? "--" + content_type.substr(pos_boundary + 9)
        : "";

    size_t pos = 0;
    // 循环处理每个 part（表单项或文件）
    while ((pos = body.find(boundary, pos)) != std::string::npos) {
        size_t part_start = pos + boundary.length();
        // 检查是否到达结尾
        if (body.substr(part_start, 2) == "--") break;
        part_start += 2;

        // 查找下一个 boundary，确定当前 part 的结束位置
        size_t part_end = body.find(boundary, part_start);
        if (part_end == std::string::npos) break;

        // 截取当前 part 内容
        std::string part = body.substr(part_start, part_end - part_start);

        // 查找头部和内容的分隔符
        size_t header_end = part.find("\r\n\r\n");
        if (header_end == std::string::npos) continue;

        // 解析头部和内容
        std::string headers = part.substr(0, header_end);
        std::string content = part.substr(header_end + 4);
        
        // 对于文件内容，不要去除末尾字符，保持原始二进制数据
        // 只有在非文件字段时才去除换行符

        std::string name, filename;
        std::istringstream header_stream(headers);
        std::string line;
        // 解析头部，获取字段名和文件名
        while (std::getline(header_stream, line)) {
            if (line.find("Content-Disposition") != std::string::npos) {
                size_t name_pos = line.find("name=\"");
                if (name_pos != std::string::npos) {
                    size_t name_end = line.find("\"", name_pos + 6);
                    name = line.substr(name_pos + 6, name_end - name_pos - 6);
                }
                size_t filename_pos = line.find("filename=\"");
                if (filename_pos != std::string::npos) {
                    size_t filename_end = line.find("\"", filename_pos + 10);
                    filename = line.substr(filename_pos + 10, filename_end - filename_pos - 10);
                }
            }
        }

	    // TicketReproduce ticketreproduce; // 问题复现工单结构体
        // 如果是文件，则保存到磁盘，并记录路径
        if (!filename.empty()) {
            std::string saved_path;
            //if (saveFile(filename, content, saved_path)) {
            //    result.saved_files.push_back(saved_path);
            //}
			// 文件内容存入 ticketreproduce.attachment（保持原始二进制数据）

            // 生成唯一文件名
            std::string uniqueFileName = generateUniqueFileName(filename);
			ticketreproduce.attachment.file = content;
			ticketreproduce.attachment.fileName = uniqueFileName;
            LOG_DEBUG("function:parseMultipartForm 文件名:%s,唯一文件名:%s\n",ticketreproduce.attachment.fileName.c_str(),uniqueFileName.c_str());
            //保存文件
            IniReader config;
            if (!config.load("config.ini")) {
                    std::cerr << "无法读取 config.ini 文件\n";
                // return false;
            }
            if(ticketreproduce.attachment.fileName != "")
            {
                std::string filePath = config.getString("storage","upload_dir_dongle_dir") + ticketreproduce.attachment.fileName;
                std::fstream ofs(filePath.c_str(),std::ios::binary | std::ios::out);
                if(!ofs.is_open())
                {
                    LOG_ERROR("function:saveUploadFile 文件打开失败!filePath:%s\n",filePath.c_str());
                    ofs.close();
                }else{
                    ofs.write(ticketreproduce.attachment.file.c_str(),ticketreproduce.attachment.file.size());
                    ofs.close();
                }
            }

        }
        // 如果是普通字段，则保存到 fields
        else if (!name.empty()) {
            // 对于普通表单字段，去除末尾的换行符
            content.erase(content.find_last_not_of("\r\n") + 1);
            result.fields[name].push_back(content);
        }

        // 移动到下一个 part
        pos = part_end;
    }
    return result; // 返回解析结果
}


void EncryptionKeyController::registerRoutes(crow::App<crow::CORSHandler>& app) {
    // 获取加密锁列表
    CROW_ROUTE(app, "/dongle/list").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 调用加密锁服务获取加密锁列表
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
                {"message", "获取加密锁列表成功"},
                {"list", dongleArray},
                {"total", dongleArray.size()}
            }}
        };
        return crow::response{ resp.dump() };
        }));

    // 获取特定加密锁的历史记录
    CROW_ROUTE(app, "/dongle/history").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        // 获取 GET 参数 dongleId
        auto params = crow::query_string(req.url_params);
        std::string dongleId = params.get("dongleId") ? params.get("dongleId") : "";

        if (dongleId.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：dongleId"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }

        // 获取历史记录
        auto historyList = encryptionKeyService_->getEncryptionKeyHistoryWithAuthRecordByEK(dongleId);

        nlohmann::json historyArray = nlohmann::json::array();
        for (const auto& historyPair : historyList) {
            const auto& history = historyPair.first;   // EncryptionKeyHistory: [outTime, inTime, clientName]
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
                {"outTime",    history.size() > 0 ? history[0] : "未知"},
                {"inTime",     history.size() > 1 ? history[1] : "未知"},
                {"clientName", history.size() > 2 ? history[2] : "暂无客户"},
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
    }));

    // 创建加密锁
    CROW_ROUTE(app, "/dongle/create").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
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

             LOG_DEBUG("创建加密锁，shellCode: %s, shellSerial: %s\n", 
                   shellCode.c_str(), shellSerial.c_str());

            // 调用服务层创建加密锁
            bool success = encryptionKeyService_->createEncryptionKey(shellCode, shellSerial);
            if(success)
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"success", true},
                        {"message", "加密锁创建成功"}
                    }}
                };
                return crow::response{ resp.dump() };
            }
            else{
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "加密锁创建失败"},
                    {"data", {
                        {"success", false},
                        {"message", "加密锁创建失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }

        } catch (const std::exception& e) {
            LOG_ERROR("创建加密锁失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));

    // 更新加密锁信息
    CROW_ROUTE(app, "/delivery/dongles/update").methods("PUT"_method)
        (withAspect([this](const crow::request& req) {
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

             LOG_DEBUG("更新加密锁信息，dongleId: %d, shellCode: %s, shellSerial: %s\n", 
                   dongleIdInt, shellCode.c_str(), shellSerial.c_str());

            // 调用服务层更新加密锁信息
            bool success = encryptionKeyService_->updateEncryptionKey(dongleIdInt, shellCode, shellSerial);
            
            if(success)
            {
                nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"success", true},
                    {"message", "加密锁信息更新成功"}
                }}
            };
            
            return crow::response{ resp.dump() };
            }
            else
            {
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", "加密锁信息更新失败"},
                    {"data", {
                        {"success", false},
                        {"message", "加密锁信息更新失败"}
                    }}
                };
                return crow::response{ resp.dump() };
            }


        } catch (const std::exception& e) {
            LOG_ERROR("更新加密锁信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));

    // 交付外壳
    CROW_ROUTE(app, "/shell/deliver").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体中的多部分表单数据
            const std::string content_type = req.get_header_value("Content-Type");
            
            TicketReproduce ticketreproduce; // 问题复现工单结构体
            MultipartResult result = parseMultipartForm(content_type, req.body,ticketreproduce);
            // 解析请求体
            // nlohmann::json reqData = nlohmann::json::parse(req.body);

            
            // 参数验证
            // if (!reqData.contains("clientName") || !reqData.contains("shellNumber") || 
            //     !reqData.contains("deviceType") || !reqData.contains("deviceNote")
            //     ) {
            //     nlohmann::json resp = {
            //         {"status", 1},
            //         {"error", "缺少必要参数：clientName、shellNumber、deviceType、deviceNote、"},
            //         {"data", {}}
            //     };
            //     return crow::response(400, resp.dump());
            // }
            std::string clientName = getField(result, "clientName");
            std::string shellNumber = getField(result, "shellNumber");
            std::string deviceType = getField(result, "deviceType");
            std::string deviceNote = getField(result, "deviceNote");
            std::string contractName = getField(result, "contractName");
            std::string contractNumber = getField(result, "contractNumber");
            std::string pdfUrl = ticketreproduce.attachment.fileName;

             LOG_DEBUG("交付外壳，clientName: %s, shellNumber: %s, deviceType: %s, deviceNote: %s, contractName: %s, contractNumber: %s, pdfUrl: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), deviceType.c_str(), deviceNote.c_str(), contractName.c_str(), contractNumber.c_str(), pdfUrl.c_str());

            // 调用服务层交付外壳
            bool success = encryptionKeyService_->deliveryOperation(clientName, shellNumber, deviceType, deviceNote, contractName, contractNumber, pdfUrl);

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
            LOG_ERROR("交付外壳失败: %s\n", e.what());
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


             LOG_DEBUG("归还外壳，clientName: %s, shellNumber: %s, inTime: %s, outTime: %s, operationType: %s, remark: %s\n", 
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
            LOG_ERROR("归还外壳失败: %s\n", e.what());
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
        (withAspect([this](const crow::request& req) {
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

             LOG_DEBUG("新建授权信息，clientName: %s, shellNumber: %s, authId: %s, authType: %s, startDate: %s, endDate: %s, authNote: %s\n", 
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
            LOG_ERROR("新建授权信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));
        
    // 获取可交付的外壳号列表
    CROW_ROUTE(app, "/delivery/available-shells").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
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
        }));

    // 更新外壳号信息
    CROW_ROUTE(app, "/shell/update").methods("PUT"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        try {
            // 解析请求体中的多部分表单数据
            const std::string content_type = req.get_header_value("Content-Type");
            
            TicketReproduce ticketreproduce; // 问题复现工单结构体
            MultipartResult result = parseMultipartForm(content_type, req.body,ticketreproduce);
            // 解析请求体
            // nlohmann::json reqData = nlohmann::json::parse(req.body);
            
            // 参数验证
            // if (!reqData.contains("clientName") || !reqData.contains("shellNumber") || 
            //     !reqData.contains("deviceType") || !reqData.contains("deviceNote")
            //     ) {
            //     nlohmann::json resp = {
            //         {"status", 1},
            //         {"error", "缺少必要参数：clientName、shellNumber、deviceType 或 deviceNote"},
            //         {"data", {}}
            //     };
            //     return crow::response(400, resp.dump());
            // }

            std::string clientName = getField(result, "clientName");
            std::string shellNumber = getField(result, "shellNumber");
            std::string deviceType = getField(result, "deviceType");
            std::string deviceNote = getField(result, "deviceNote");
            std::string contractName = getField(result, "contractName");
            std::string contractNumber = getField(result, "contractNumber");
            std::string pdfUrl = ticketreproduce.attachment.fileName;

             LOG_DEBUG("更新外壳号信息，clientName: %s, shellNumber: %s, deviceType: %s, deviceNote: %s, contractName: %s, contractNumber: %s, pdfUrl: %s\n", 
                   clientName.c_str(), shellNumber.c_str(), deviceType.c_str(), deviceNote.c_str(), contractName.c_str(), contractNumber.c_str(), pdfUrl.c_str());

            // 调用服务层更新外壳号信息
            bool success = encryptionKeyService_->updateShellDeviceInfo(clientName, shellNumber, deviceType, deviceNote, contractName, contractNumber, pdfUrl);

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
            LOG_ERROR("更新外壳号信息失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));

    // 批量更新授权截止日期
    CROW_ROUTE(app, "/auth/batch-update").methods("PUT"_method)
        (withAspect([this](const crow::request& req) {
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

             LOG_DEBUG("批量更新授权截止日期，clientName: %s, 更新数量: %zu\n", 
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
            LOG_ERROR("批量更新授权截止日期失败: %s\n", e.what());
            nlohmann::json resp = {
                {"status", 1},
                {"error", "参数解析失败或服务器内部错误"},
                {"data", {}}
            };
            return crow::response(500, resp.dump());
        }
        }));


}