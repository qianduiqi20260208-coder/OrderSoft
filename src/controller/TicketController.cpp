#include "TicketController.h"
#include <nlohmann/json.hpp>
#include "IniReader.h"
#include <jwt_utils.h>

TicketController::TicketController(std::shared_ptr<ITicketService> sp):ticketService(sp)
{
    // 初始化 mock 客户列表
    customerList_ = { "zhangsan", "lisi"};

    // 初始化 mock 外壳号列表
    shellNumberList_ = { "qwer1234", "asdf1234", "A53000000003"};

    // 初始化 mock 授权ID列表
    authIDList_ = { "333444555", "1", "2"};
}

// 解析 multipart/form-data 格式的表单数据
MultipartResult TicketController::parseMultipartForm(const std::string& content_type, const std::string& body)
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

crow::response TicketController::downloadTicketFile(int ticketId, const std::string& filename) {
    try {
        // 读取配置文件，获取上传目录
        IniReader config;
        if (!config.load("../../config/config.ini")) {
            printf("[error] function:downloadTicketFile 无法读取 config.ini 文件\n");
            return crow::response(500, R"({"status":0,"error":"配置文件读取失败","data":{}})");
        }
        
        // 构建文件路径，与保存时的路径格式保持一致
        std::string relativePath = "/ticket/" + filename;
        std::string filePath = config.getString("storage", "upload_dir") + relativePath;
        
        // 检查文件是否存在
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            printf("[error] function:downloadTicketFile 文件打开失败!filePath:%s\n", filePath.c_str());
            return crow::response(404, R"({"status":0,"error":"文件不存在","data":{}})");
        }
        
        // 获取文件大小
        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // 读取文件内容 - 使用更安全的二进制读取方式
        std::string content(fileSize, '\0');
        file.read(&content[0], fileSize);

        // 验证读取是否完整
        if (!file || file.gcount() != fileSize) {
            printf("[error] function:downloadTicketFile 文件读取不完整! 期望:%ld, 实际:%ld\n", 
                fileSize, file.gcount());
            return crow::response(500, R"({"status":0,"error":"文件读取失败","data":{}})");
        }

        file.close();
        
        // 根据文件扩展名设置Content-Type
        std::string contentType = "application/octet-stream";
        size_t dotPos = filename.find_last_of('.');
        if (dotPos != std::string::npos) {
            std::string extension = filename.substr(dotPos + 1);
            
            if (extension == "jpg" || extension == "jpeg") {
                contentType = "image/jpeg";
            } else if (extension == "png") {
                contentType = "image/png";
            } else if (extension == "pdf") {
                contentType = "application/pdf";
            } else if (extension == "txt") {
                contentType = "text/plain";
            } else if (extension == "doc" || extension == "docx") {
                contentType = "application/msword";
            } else if (extension == "xls" || extension == "xlsx") {
                contentType = "application/vnd.ms-excel";
            }
        }

        // 将文件内容转换为Base64编码
        std::string base64Content = base64_encode(content); // 你需要实现这个函数
        
        // 返回JSON格式，符合前端期望的格式
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"filename", filename},
                {"contentType", contentType},
                {"size", content.size()},
                {"content", base64Content}  // Base64编码的文件内容
            }}
        };
        
        crow::response response(200, resp.dump());
        response.add_header("Content-Type", "application/json");
        response.add_header("Access-Control-Allow-Origin", "*");
        return response;
        
    } catch (const std::exception& e) {
        printf("[error] function:downloadTicketFile 异常:%s\n", e.what());
        nlohmann::json errorResp = {
            {"status", 1},
            {"error", "文件下载失败"},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResp.dump());
    }
}

void TicketController::registerRoutes(crow::SimpleApp& app) {

    // 获取工单分页（可筛选）
    CROW_ROUTE(app, "/order/all").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        
        // 解析查询参数
        auto params = crow::query_string(req.url_params);
        int page = std::stoi(params.get("page") ? params.get("page") : "1");
        int pageSize = std::stoi(params.get("pageSize") ? params.get("pageSize") : "10");
        std::string orderID = params.get("orderID") ? params.get("orderID") : "";
        std::string type = params.get("type") ? params.get("type") : "";
        std::string promoterID = params.get("promoterID") ? params.get("promoterID") : "";
        std::string modelID = params.get("modelID") ? params.get("modelID") : "";
        std::string referencePriority = params.get("referencePriority") ? params.get("referencePriority") : "";
        std::string taskPriority = params.get("taskPriority") ? params.get("taskPriority") : "";
        std::string status = params.get("status") ? params.get("status") : "";
        // std::string startDate = params.get("startDate") ? params.get("startDate") : "";
        // std::string endDate = params.get("endDate") ? params.get("endDate") : "";

        // 构建筛选条件
        std::map<std::string, std::string> filter;
        
        // 根据前端传来的参数构建filter
        if (!orderID.empty()) {
            filter["id"] = orderID;  // 根据工单ID筛选
        }
        if (!type.empty()) {
            filter["type"] = type;  // 根据工单类型筛选
        }
        if (!promoterID.empty()) {
            filter["creator_id"] = promoterID;  // 根据发起人工号筛选
        }
        if (!status.empty()) {
            filter["status"] = status;  // 根据工单状态筛选
        }
        if (!referencePriority.empty()) {
            filter["priority"] = referencePriority;  // 根据参考优先级筛选
        }
        if (!taskPriority.empty()) {
            filter["task_priority"] = taskPriority;  // 根据任务优先级
        }
        if (!modelID.empty()) {
            filter["model"] = modelID;  // 根据模型名筛选
        }

        // 计算分页偏移量
        int offset = (page - 1) * pageSize;
        
        // 使用统一的方法查询（支持筛选+分页）
        auto tickets = ticketService->selectOrderByCondition_(filter, offset, pageSize);

        // 构建响应列表
        nlohmann::json list = nlohmann::json::array();
        for (const auto& ticketPtr : tickets) {
            if (ticketPtr) {
                list.push_back(ticketPtr->to_json());
            }
        }

        // 获取工单总数
        long long totalCount = ticketService->getOrderCount();

        // 构建响应
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", list},
                {"total", totalCount},
                {"page", page},
                {"pageSize", pageSize}
            }}
        };

        return crow::response{ resp.dump() };
    });

    // 提交问题复现工单（支持FormData格式上传文件）
    CROW_ROUTE(app, "/order/problem").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
		// 解析请求体中的多部分表单数据
        const std::string content_type = req.get_header_value("Content-Type");
        MultipartResult result = parseMultipartForm(content_type, req.body);
        // 检查字段
        ticketreproduce.ticketType = "问题复现"; // 工单类型
        ticketreproduce.status = "待审批"; // 工单状态
        ticketreproduce.creatorId = std::stoi(getField(result, "promoterID")); // 发起人ID
        ticketreproduce.createTime = getField(result, "startTime"); // 发起时间
        ticketreproduce.model = getField(result, "modelID"); // 关联模型
        ticketreproduce.modelVersion = getField(result, "modelVersionID"); // 关联模型版本
        ticketreproduce.coordinationId = getField(result, "coordinationID"); // 协调单ID
        ticketreproduce.content = getField(result, "description"); // 复现内容描述
        //ticketreproduce.attachment.file = body.value("files", std::vector<nlohmann::json>{});
        //ticketreproduce.attachment.fileName = body.value("files", std::vector<nlohmann::json>{});
        ticketreproduce.approverId = std::stoi(getField(result, "approverID")); // 审批人ID
        bool ok = ticketService->createTicket(ticketreproduce);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "问题复现工单提交成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "问题复现工单提交失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 提交版本迭代工单
    CROW_ROUTE(app, "/order/iter").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
		// 解析请求体
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketversion.ticketType = "版本迭代"; // 工单类型
        ticketversion.status = "待审批"; // 工单状态
        ticketversion.creatorId = std::stoi(body.value("promoterID", "")); // 发起人ID
        ticketversion.createTime = body.value("startTime", ""); // 发起时间
        ticketversion.model = body.value("modelID", "");  // 关联模型
        ticketversion.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketversion.coordinationId = body.value("coordinationID", ""); // 协调单ID
        ticketversion.updateNote = body.value("updateNotes", ""); // 更新内容
        ticketversion.packRequirement = body.value("packageRequirement", ""); // 封装要求
        ticketversion.interfaceChanged = (body.value("apiChanged", "") == "是"); // 接口是否变化
        ticketversion.approverId = std::stoi(body.value("approverID", "")); // 审批人ID

        bool ok = ticketService->createTicket(ticketversion);

        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "版本迭代工单提交成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "版本迭代工单提交失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 提交交付发送工单
    CROW_ROUTE(app, "/order/deliver").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        ticketdelivery.ticketType = "交付发送"; // 工单类型
        ticketdelivery.status = "待审批"; // 工单状态
        ticketdelivery.creatorId = std::stoi(body.value("promoterID", "")); // 发起人ID
        ticketdelivery.createTime = body.value("startTime", ""); // 发起时间
        ticketdelivery.model = body.value("modelID", ""); // 关联模型
        ticketdelivery.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketdelivery.targetClient = body.value("targetCustomer", ""); // 目标客户
        ticketdelivery.validatedByCAE = (body.value("isCAEChecked", "") == "是"); // 是否经过CAE检查
        ticketdelivery.sensitiveInfo = body.value("hasSensitiveInfo", ""); // 是否包含敏感信息
        ticketdelivery.approverId = std::stoi(body.value("approverID", "")); // 审批人ID

        bool ok = ticketService->createTicket(ticketdelivery);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "交付发送工单提交成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "交付发送工单提交失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 提交迭代+交付工单
    CROW_ROUTE(app, "/order/iter-deliver").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        ticketpackage.ticketType = "直接封装+发送"; // 工单类型
        ticketpackage.status = "待审批"; // 工单状态
        ticketpackage.creatorId = std::stoi(body.value("promoterID", "")); // 发起人ID
        ticketpackage.createTime = body.value("startTime", ""); // 发起时间
        ticketpackage.model = body.value("modelID", ""); // 关联模型
        ticketpackage.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketpackage.coordinationId = body.value("coordinationID", ""); // 协调单ID
        ticketpackage.updateNote = body.value("updateNotes", ""); // 更新内容
        ticketpackage.packRequirement = body.value("packageRequirement", ""); // 封装要求
        ticketpackage.interfaceChanged = (body.value("apiChanged", "") == "是"); // 接口是否变化
        ticketpackage.targetClient = body.value("targetCustomer", ""); // 目标客户
        ticketpackage.validatedByCAE = (body.value("isCAEChecked", "") == "是"); // 是否经过CAE检查
        ticketpackage.sensitiveInfo = body.value("hasSensitiveInfo", ""); // 是否包含敏感信息
        ticketpackage.approverId = std::stoi(body.value("approverID", "")); // 审批人ID


        bool ok = ticketService->createTicket(ticketpackage);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "版本迭代+交付发送工单提交成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "版本迭代+交付发送工单提交失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 提交功能开发工单
    CROW_ROUTE(app, "/order/dev").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        ticketfeature.ticketType = "功能开发"; // 工单类型
        ticketfeature.status = "待审批"; // 工单状态
        ticketfeature.creatorId = std::stoi(body.value("promoterID", "")); // 发起人ID
        ticketfeature.createTime = body.value("startTime", ""); // 发起时间
        ticketfeature.model = body.value("modelID", ""); // 关联模型
        ticketfeature.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketfeature.featureInit = body.value("featureDesc", ""); // 功能描述
        ticketfeature.approverId = std::stoi(body.value("approverID", "")); // 审批人ID

        bool ok = ticketService->createTicket(ticketfeature);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "功能开发工单提交成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "功能开发工单提交失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 提交其他工单
    CROW_ROUTE(app, "/order/other").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        ticketother.ticketType = "其他"; // 工单类型
        ticketother.status = "待审批"; // 工单状态
        ticketother.creatorId = std::stoi(body.value("promoterID", "")); // 发起人ID
        ticketother.createTime = body.value("startTime", ""); // 发起时间
        ticketother.model = body.value("modelID", ""); // 关联模型
        ticketother.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketother.description = body.value("contentDesc", ""); // 内容描述
        ticketother.approverId = std::stoi(body.value("approverID", "")); // 审批人ID


        bool ok = ticketService->createTicket(ticketother);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "其他工单提交成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "其他工单提交失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 工单审批同意
    CROW_ROUTE(app, "/order/approve").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "待分发"; // 更新工单状态为待分发
        ticket.approverId = std::stoi(body.value("approverID", "")); // 审批人ID
        ticket.approvedTime = body.value("approveTime", ""); // 审批时间
        ticket.priorityHint = body.value("referencePriority", ""); // 参考优先级
        ticket.distributorId = std::stoi(body.value("distributorID", "")); // 分发人ID

        bool ok = ticketService->approveTicket(ticket);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "审批已同意"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "审批同意失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 审批环节拒绝
    CROW_ROUTE(app, "/order/reject-approve").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "已退回"; // 更新工单状态为已退回
        ticket.priorityHint.clear(); // 清空参考优先级
        ticket.approverId = std::stoi(body.value("approverID", "")); // 审批人ID
        ticket.approvedTime = body.value("approveTime", ""); // 审批时间
        ticket.rejectReason = body.value("rejectReason", ""); // 拒绝原因 

        bool ok = ticketService->approveTicket(ticket);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "审批已拒绝"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "审批拒绝失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 工单分发同意
    CROW_ROUTE(app, "/order/distribute").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "进行中";  // 更新工单状态为进行中
        ticket.distributorId = std::stoi(body.value("distributorID", "")); // 分发人ID
        ticket.distributedTime = body.value("distributeTime", ""); // 分发时间
        ticket.priorityTask = body.value("taskPriority", ""); // 任务优先级
        ticket.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->dispatchTicket(ticket);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "分发已同意"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "分发同意失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 分发环节拒绝
    CROW_ROUTE(app, "/order/reject-distribute").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "已退回"; // 更新工单状态为已退回
        ticket.priorityTask.clear(); // 清空任务优先级
        ticket.distributorId = std::stoi(body.value("distributorID", "")); // 分发人ID
        ticket.distributedTime = body.value("distributeTime", ""); // 分发时间
        ticket.rejectReason = body.value("rejectReason", ""); // 拒绝原因

        bool ok = ticketService->dispatchTicket(ticket);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "分发已拒绝"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "分发拒绝失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 问题复现工单完成
    CROW_ROUTE(app, "/order/finish-problem").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketreproduce.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketreproduce.status = "已完成"; // 更新工单状态为已完成
        ticketreproduce.ticketType = "问题复现"; // 工单类型 显示指定
        ticketreproduce.completedTime = body.value("finishTime", ""); // 完成时间
        ticketreproduce.phenomenon = body.value("finishPhenomenon", ""); // 复现现象
        ticketreproduce.remark = body.value("finishRemark", ""); // 备注
        ticketreproduce.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->completeTicket(ticketreproduce);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "问题复现工单填写成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "问题复现工单填写失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 版本迭代工单完成
    CROW_ROUTE(app, "/order/finish-iter").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketversion.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketversion.status = "已完成"; // 更新工单状态为已完成
        ticketversion.ticketType = "版本迭代"; // 工单类型 显示指定
        ticketversion.completedTime = body.value("finishTime", ""); // 完成时间
        ticketversion.newModelVersion = body.value("finishModelVersion", ""); // 升级后模型版本
        ticketversion.remark = body.value("finishRemark", ""); // 备注
        ticketversion.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->completeTicket(ticketversion);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "版本迭代工单填写成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "版本迭代工单填写失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 交付发送工单完成
    CROW_ROUTE(app, "/order/finish-deliver").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketdelivery.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketdelivery.status = "已完成"; // 更新工单状态为已完成
        ticketdelivery.ticketType = "交付发送"; // 工单类型 显示指定
        ticketdelivery.completedTime = body.value("finishTime", ""); // 完成时间
        ticketdelivery.encrypted = (body.value("isEncrypted", "") == "是"); // 是否加密
        ticketdelivery.licenseId = body.value("finishAuthId", ""); // 授权ID
        ticketdelivery.dongleId = body.value("finishShellNo", ""); // 外壳号
        ticketdelivery.remark = body.value("finishRemark", ""); // 备注
        ticketdelivery.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->completeTicket(ticketdelivery);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "交付发送工单填写成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "交付发送工单填写失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 版本迭代+交付发送工单完成
    CROW_ROUTE(app, "/order/finish-iter-deliver").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketpackage.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketpackage.ticketType = "直接封装+发送"; // 工单类型 显示指定
        ticketpackage.status = "已完成"; // 更新工单状态为已完成
        ticketpackage.completedTime = body.value("finishTime", ""); // 完成时间
        ticketpackage.newModelVersion = body.value("finishModelVersion", ""); // 升级后模型版本ID
        ticketpackage.encrypted = (body.value("isEncrypted", "") == "是"); // 是否加密
        ticketpackage.license = body.value("finishAuthId", ""); // 授权ID
        ticketpackage.dongle = body.value("finishShellNo", ""); // 外壳号
        ticketpackage.remark = body.value("finishRemark", ""); // 备注
        ticketpackage.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->completeTicket(ticketpackage);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "版本迭代+交付发送工单填写成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "版本迭代+交付发送工单填写失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 功能开发工单完成
    CROW_ROUTE(app, "/order/finish-dev").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketfeature.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketfeature.status = "已完成"; // 更新工单状态为已完成
        ticketfeature.ticketType = "功能开发"; // 工单类型 显示指定
        ticketfeature.completedTime = body.value("finishTime", ""); // 完成时间
        ticketfeature.newModelVersion = body.value("finishModelVersionId", ""); // 升级后模型版本ID
        ticketfeature.featureFinal = body.value("finishFeatureDesc", ""); // 完成功能描述
        ticketfeature.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->completeTicket(ticketfeature);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "功能开发工单填写成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "功能开发工单填写失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 其他工单完成
    CROW_ROUTE(app, "/order/finish-other").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 检查字段
        ticketother.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketother.status = "已完成"; // 更新工单状态为已完成
        ticketother.ticketType = "其他"; // 工单类型 显示指定
        ticketother.completedTime = std::stoi(body.value("finishTime", "")); // 完成时间
        ticketother.remark = body.value("finishRemarkOther", ""); // 完成备注
        ticketother.executorId = std::stoi(body.value("executorID", "")); // 执行人ID

        bool ok = ticketService->completeTicket(ticketother);
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "其他工单填写成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "其他工单填写失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });

    // 工单流转
    CROW_ROUTE(app, "/order/transfer").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":0,"error":"Invalid JSON","data":{}})");
        }

        // 清空并重新填充数组
        ticketexecutor.executor.clear();
        ticketexecutor.reason.clear();
        ticketexecutor.timestamp.clear();

        // 检查字段
        ticketexecutor.ticketId = std::stoi(body.value("orderID", "")); // 工单ID
        ticketexecutor.executor.push_back(body.value("executorID", "")); // [0] 当前执行人ID
        ticketexecutor.executor.push_back(body.value("transferExecutorID", "")); // [1] 流转目标执行人ID
        ticketexecutor.reason.push_back(body.value("transferReason", "")); // [0] 流转原因
        ticketexecutor.timestamp.push_back(body.value("transferTime", "")); // [0] 流转时间

        bool ok = ticketService->orderTransfer(ticketexecutor);
        
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "流转成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "流转失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        });


    // 获取客户信息列表
    CROW_ROUTE(app, "/customer/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        std::vector<std::string> clientList = ticketService->getClient();

        nlohmann::json customers = clientList;
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", customers}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取外壳号列表
    CROW_ROUTE(app, "/order/shell-numbers").methods("GET"_method)
        ([this](const crow::request& req) {
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

        nlohmann::json shellNumber = shellNumberList_;
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", shellNumber}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 获取授权ID列表
    CROW_ROUTE(app, "/order/auth-ids").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 从查询参数中获取shellNumber
        std::string shellNumber = "";
        auto params = crow::query_string(req.url_params);
        if (params.get("shellNumber") != nullptr) {
            shellNumber = params.get("shellNumber");
        }

        // 参数验证
        if (shellNumber.empty()) {
            nlohmann::json resp = {
                {"status", 0},
                {"error", "缺少必要参数：targetCustomer"},
                {"data", {}}
            };
            return crow::response(400, resp.dump());
        }
        
        // 根据不同客户返回不同的Mock授权ID数据
        nlohmann::json authIDList = nlohmann::json::array();
        
        if (shellNumber == "qwer1234") {
            authIDList = {
                {
                    {"authId", "333444555"},
                    {"endDate", "2025-10-01"},
                    {"deviceType", "FTD"},
                    {"description", "张三个人授权"}
                },
                {
                    {"authId", "2025071005"},
                    {"endDate", "2025-05-20"},
                    {"deviceType", "FFS"},
                    {"description", "张三测试授权"}
                }
            };
        }
        else if (shellNumber == "asdf1234") {
            authIDList = {
                {
                    {"authId", "333444555"},
                    {"endDate", "2025-07-15"},
                    {"deviceType", "lab"},
                    {"description", "李四开发授权"}
                },
                {
                    {"authId", "2025071007"},
                    {"endDate", "2024-12-01"},
                    {"deviceType", "FTD"},
                    {"description", "李四临时授权"}
                },
                {
                    {"authId", "2025071003"},
                    {"endDate", "2025-12-31"},
                    {"deviceType", "IPT"},
                    {"description", "华模测试授权"}
                }
            };
        }
    
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", authIDList}
            }}
        };
        return crow::response{ resp.dump() };
        });

    // 工单附件文件下载接口
    CROW_ROUTE(app, "/files/ticket/<int>/<string>").methods("GET"_method)
        ([this](const crow::request& req, int ticketId, const std::string& filename) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        
        printf("[info] 文件下载请求 - ticketId:%d, filename:%s\n", ticketId, filename.c_str());
        return downloadTicketFile(ticketId, filename);
        });
}
