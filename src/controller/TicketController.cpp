#include "TicketController.h"
#include <nlohmann/json.hpp>
#include "IniReader.h"
#include <jwt_utils.h>
#include "Log.h"
#include "Logger.h"



std::string url_decode(const std::string& str) {
    std::string ret;
    char ch;
    int i, ii;
    for (i = 0; i < str.length(); i++) {
        if (str[i] == '%') {
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &ii);
            ch = static_cast<char>(ii);
            ret += ch;
            i = i + 2;
        } else if (str[i] == '+') {
            ret += ' ';
        } else {
            ret += str[i];
        }
    }
    return ret;
}

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
MultipartResult TicketController::parseMultipartForm(const std::string& content_type, const std::string& body,TicketReproduce& ticketreproduce)
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
        if (!config.load("config.ini")) {
            LOG_ERROR("function:downloadTicketFile 无法读取 config.ini 文件\n");
            return crow::response(500, R"({"status":1,"error":"配置文件读取失败","data":{}})");
        }
        
        // 构建文件路径，与保存时的路径格式保持一致
        std::string relativePath = "/ticket/" + filename;
        std::string filePath = config.getString("storage", "upload_dir") + relativePath;
        
        // 检查文件是否存在
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("function:downloadTicketFile 文件打开失败!filePath:%s\n", filePath.c_str());
            return crow::response(404, R"({"status":1,"error":"文件不存在","data":{}})");
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
            LOG_ERROR("function:downloadTicketFile 文件读取不完整! 期望:%ld, 实际:%ld\n", 
                fileSize, file.gcount());
            return crow::response(500, R"({"status":1,"error":"文件读取失败","data":{}})");
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
        return response;
        
    } catch (const std::exception& e) {
        LOG_ERROR("function:downloadTicketFile 异常:%s\n", e.what());
        nlohmann::json errorResp = {
            {"status", 1},
            {"error", "文件下载失败"},
            {"data", nlohmann::json::object()}
        };
        return crow::response(500, errorResp.dump());
    }
}

void TicketController::registerRoutes(crow::App<crow::CORSHandler>& app) {

    // 获取工单分页（可筛选）
    CROW_ROUTE(app, "/order/all").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        // if (!checkToken(req)) {
        //     return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        // }
        
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
        std::string filterMineFlag = params.get("filterMineFlag") ? params.get("filterMineFlag") : "";
        std::string userId = params.get("userId") ? params.get("userId") : "";
        std::string startDate = params.get("startDate") ? params.get("startDate") : "";
        std::string endDate = params.get("endDate") ? params.get("endDate") : "";

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
        if (!filterMineFlag.empty()) {
            filter["filterMineFlag"] = filterMineFlag;  // 根据我的工单标志筛选
        }
        if (!userId.empty()) {
            filter["userId"] = userId;  // 当前用户ID
        }
        if (!startDate.empty()) {
            filter["startDate"] = startDate;  // 目标交付时间开始日期
        }
        if (!endDate.empty()) {
            filter["endDate"] = endDate;  // 目标交付时间结束日期
        }

        // 计算分页偏移量
        int offset = (page - 1) * pageSize;
        
        // 使用新的详细查询方法（支持筛选+分页，返回格式与待办查询一致）
        auto workOrders = ticketService->selectOrderByConditionWithDetails(filter, offset, pageSize);

        // 直接使用返回的JSON数组
        nlohmann::json list = workOrders;

        // 获取工单总数
        long long totalCount = ticketService->getOrderCount(filter);

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
    }));

    // 提交问题复现工单（支持FormData格式上传文件）
    CROW_ROUTE(app, "/order/problem").methods("POST"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
		// 解析请求体中的多部分表单数据
        const std::string content_type = req.get_header_value("Content-Type");
        
	    TicketReproduce ticketreproduce; // 问题复现工单结构体
        MultipartResult result = parseMultipartForm(content_type, req.body,ticketreproduce);
        // 检查字段
        ticketreproduce.ticketType = "问题复现"; // 工单类型
        ticketreproduce.status = "待审批"; // 工单状态
        ticketreproduce.creatorId = (getField(result, "promoterID")); // 发起人ID
        ticketreproduce.createTime = getField(result, "startTime"); // 发起时间
        ticketreproduce.model = getField(result, "modelID"); // 关联模型
        ticketreproduce.modelVersion = getField(result, "modelVersionID"); // 关联模型版本
        ticketreproduce.coordinationId = getField(result, "coordinationID"); // 协调单ID
        ticketreproduce.content = getField(result, "description"); // 复现内容描述
        ticketreproduce.createRemark = getField(result, "create_remark"); // 创建备注
        ticketreproduce.approverId = (getField(result, "approverID")); // 审批人ID
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
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
		// 解析请求体
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketVersion ticketversion; // 定义本地ticketversion变量

        // 检查字段
        ticketversion.ticketType = "版本迭代"; // 工单类型
        ticketversion.status = "待审批"; // 工单状态
        ticketversion.creatorId = (body.value("promoterID", "")); // 发起人ID
        ticketversion.createTime = body.value("startTime", ""); // 发起时间
        ticketversion.model = body.value("modelID", "");  // 关联模型
        ticketversion.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketversion.matlab_version = body.value("completeModelVersion", ""); // 期望完成后的模型版本
        ticketversion.coordinationId = body.value("coordinationID", ""); // 协调单ID
        ticketversion.updateNote = body.value("updateNotes", ""); // 更新内容
        ticketversion.packRequirement = body.value("packageRequirement", ""); // 封装要求
        ticketversion.interfaceChanged = (body.value("apiChanged", "") == "是"); // 接口是否变化
        ticketversion.createRemark = body.value("create_remark", ""); // 创建备注
        ticketversion.approverId = (body.value("approverID", "")); // 审批人ID

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
        }));

    // 提交交付发送工单
    CROW_ROUTE(app, "/order/deliver").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketDelivery ticketdelivery; // 定义本地ticketdelivery变量

        ticketdelivery.ticketType = "交付发送"; // 工单类型
        ticketdelivery.status = "待审批"; // 工单状态
        ticketdelivery.creatorId = (body.value("promoterID", "")); // 发起人ID
        ticketdelivery.createTime = body.value("startTime", ""); // 发起时间
        ticketdelivery.model = body.value("modelID", ""); // 关联模型
        ticketdelivery.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketdelivery.targetClient = body.value("targetCustomer", ""); // 目标客户
        ticketdelivery.validatedByCAE = (body.value("isCAEChecked", "") == "是"); // 是否经过CAE检查
        ticketdelivery.sensitiveInfo = body.value("hasSensitiveInfo", ""); // 是否包含敏感信息
        ticketdelivery.createRemark = body.value("create_remark", ""); // 创建备注
        ticketdelivery.approverId = (body.value("approverID", "")); // 审批人ID

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
        }));

    // 提交迭代+交付工单
    CROW_ROUTE(app, "/order/iter-deliver").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        // 定义本地ticketpackage变量
        TicketPackage ticketpackage;

        ticketpackage.ticketType = "直接封装+发送"; // 工单类型
        ticketpackage.status = "待审批"; // 工单状态
        ticketpackage.creatorId = (body.value("promoterID", "")); // 发起人ID
        ticketpackage.createTime = body.value("startTime", ""); // 发起时间
        ticketpackage.model = body.value("modelID", ""); // 关联模型
        ticketpackage.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        // ticketpackage.remark = body.value("completeModelVersion", ""); // 期望完成后的模型版本
        ticketpackage.matlab_version = body.value("completeModelVersion", ""); // Matlab版本
        ticketpackage.coordinationId = body.value("coordinationID", ""); // 协调单ID
        ticketpackage.updateNote = body.value("updateNotes", ""); // 更新内容
        ticketpackage.packRequirement = body.value("packageRequirement", ""); // 封装要求
        ticketpackage.interfaceChanged = (body.value("apiChanged", "") == "是"); // 接口是否变化
        ticketpackage.targetClient = body.value("targetCustomer", ""); // 目标客户
        ticketpackage.validatedByCAE = (body.value("isCAEChecked", "") == "是"); // 是否经过CAE检查
        ticketpackage.sensitiveInfo = body.value("hasSensitiveInfo", ""); // 是否包含敏感信息
        ticketpackage.createRemark = body.value("create_remark", ""); // 创建备注
        ticketpackage.approverId = (body.value("approverID", "")); // 审批人ID
        ticketpackage.ticketType = "直接封装+发送"; // 工单类型 


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
        }));

    // 提交功能开发工单
    CROW_ROUTE(app, "/order/dev").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketFeature ticketfeature; // 定义本地ticketfeature变量

        ticketfeature.ticketType = "功能开发"; // 工单类型
        ticketfeature.status = "待审批"; // 工单状态
        ticketfeature.creatorId = (body.value("promoterID", "")); // 发起人ID
        ticketfeature.createTime = body.value("startTime", ""); // 发起时间
        ticketfeature.model = body.value("modelID", ""); // 关联模型
        ticketfeature.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketfeature.featureFinal = body.value("completeModelVersion", ""); // 期望完成后的模型版本
        ticketfeature.matlab_version = body.value("matlab_version", ""); // MATLAB版本
        ticketfeature.featureInit = body.value("featureDesc", ""); // 功能描述
        ticketfeature.createRemark = body.value("create_remark", ""); // 创建备注
        ticketfeature.approverId = (body.value("approverID", "")); // 审批人ID

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
        }));

    // 提交其他工单
    CROW_ROUTE(app, "/order/other").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketOther ticketother;

        ticketother.ticketType = "其他"; // 工单类型
        ticketother.status = "待审批"; // 工单状态
        ticketother.creatorId = (body.value("promoterID", "")); // 发起人ID
        ticketother.createTime = body.value("startTime", ""); // 发起时间
        ticketother.model = body.value("modelID", ""); // 关联模型
        ticketother.modelVersion = body.value("modelVersionID", ""); // 关联模型版本
        ticketother.description = body.value("contentDesc", ""); // 内容描述
        ticketother.createRemark = body.value("create_remark", ""); // 创建备注
        ticketother.approverId = (body.value("approverID", "")); // 审批人ID


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
        }));

    // 工单审批同意
    CROW_ROUTE(app, "/order/approve").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }
        Ticket ticket;
        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "待分发"; // 更新工单状态为待分发
        ticket.approverId = (body.value("approverID", "")); // 审批人ID
        ticket.approvedTime = body.value("approveTime", ""); // 审批时间
        ticket.priorityHint = body.value("referencePriority", ""); // 参考优先级
        ticket.distributorId = (body.value("distributorID", "")); // 分发人ID
        ticket.targetDeliveryTime = (body.value("targetDeliveryTime", "")); // 预计发送时间
        ticket.ticketType = body.value("orderType", ""); // 工单类型

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
        }));

    // 审批环节拒绝
    CROW_ROUTE(app, "/order/reject-approve").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }
        Ticket ticket;
        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "已退回"; // 更新工单状态为已退回
        ticket.priorityHint.clear(); // 清空参考优先级
        ticket.approverId = (body.value("approverID", "")); // 审批人ID
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
        }));

    // 工单分发同意
    CROW_ROUTE(app, "/order/distribute").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }
        Ticket ticket;
        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "进行中";  // 更新工单状态为进行中
        ticket.distributorId = (body.value("distributorID", "")); // 分发人ID
        ticket.distributedTime = body.value("distributeTime", ""); // 分发时间
        ticket.priorityTask = body.value("taskPriority", ""); // 任务优先级
        ticket.executorId = (body.value("executorID", "")); // 执行人ID
        ticket.ticketType = body.value("orderType", ""); // 工单类型

        // 获取当前登录用户的account
        std::string account = getAccountFromToken(req);
        if (account.empty()) {
            return crow::response(401, R"({"status":1,"error":"无法获取用户信息","data":{}})");
        }
        
        bool ok = ticketService->dispatchTicket(ticket, account);
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
        }));

    // 分发环节拒绝
    CROW_ROUTE(app, "/order/reject-distribute").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }
        Ticket ticket;
        // 检查字段
        ticket.id = std::stoi(body.value("orderID", "")); // 工单ID
        ticket.status = "已退回"; // 更新工单状态为已退回
        ticket.priorityTask.clear(); // 清空任务优先级
        ticket.distributorId = (body.value("distributorID", "")); // 分发人ID
        ticket.distributedTime = body.value("distributeTime", ""); // 分发时间
        ticket.rejectReason = body.value("rejectReason", ""); // 拒绝原因

        // 获取当前登录用户的account
        std::string account = getAccountFromToken(req);
        if (account.empty()) {
            return crow::response(401, R"({"status":1,"error":"无法获取用户信息","data":{}})");
        }
        
        bool ok = ticketService->dispatchTicket(ticket, account);
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
        }));

    // 工单完成
    CROW_ROUTE(app, "/order/finish-problem").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

	    TicketReproduce ticketreproduce; // 问题复现工单结构体
        // 检查字段
        ticketreproduce.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketreproduce.status = "已完成"; // 更新工单状态为已完成
        ticketreproduce.ticketType = "问题复现"; // 工单类型 显示指定
        ticketreproduce.completedTime = body.value("finishTime", ""); // 完成时间
        ticketreproduce.phenomenon = body.value("finishPhenomenon", ""); // 复现现象
        ticketreproduce.remark = body.value("finishRemark", ""); // 备注
        ticketreproduce.executorId = (body.value("executorID", "")); // 执行人ID

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
        }));

    // 版本迭代工单完成
    CROW_ROUTE(app, "/order/finish-iter").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketVersion ticketversion;

        // 检查字段
        ticketversion.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketversion.model = body.value("modelID", ""); // 关联模型
        ticketversion.status = "已完成"; // 更新工单状态为已完成
        ticketversion.ticketType = "版本迭代"; // 工单类型 显示指定
        ticketversion.completedTime = body.value("finishTime", ""); // 完成时间
        ticketversion.newModelVersion = body.value("finishModelVersion", ""); // 升级后模型版本
        ticketversion.remark = body.value("finishRemark", ""); // 备注
        ticketversion.executorId = (body.value("executorID", "")); // 执行人ID

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
        }));

    // 交付发送工单完成
    CROW_ROUTE(app, "/order/finish-deliver").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        // 定义本地ticketdelivery变量
        TicketDelivery ticketdelivery;

        // 检查字段
        ticketdelivery.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketdelivery.status = "已完成"; // 更新工单状态为已完成
        ticketdelivery.ticketType = "交付发送"; // 工单类型 显示指定
        ticketdelivery.completedTime = body.value("finishTime", ""); // 完成时间
        ticketdelivery.encrypted = (body.value("isEncrypted", "") == "是"); // 是否加密
        ticketdelivery.licenseId = body.value("finishAuthId", ""); // 授权ID
        // 处理authorizationId_list
        if (body.contains("authorizationId_list") && body["authorizationId_list"].is_array()) {
            std::string authIds;
            for (const auto& authId : body["authorizationId_list"]) {
                if (authId.is_string()) {
                    if (!authIds.empty()) {
                        authIds += ",";
                    }
                    authIds += authId.get<std::string>();
                }
            }
            ticketdelivery.authorizationIdList = authIds;
        }

        // 处理外壳号，支持多个外壳号以英文逗号分割
        if (body.contains("finishShellNo") && body["finishShellNo"].is_array()) {
            std::string dongleIds;
            for (const auto& shellNo : body["finishShellNo"]) {
                if (shellNo.is_string()) {
                    if (!dongleIds.empty()) {
                        dongleIds += ",";
                    }
                    dongleIds += shellNo.get<std::string>();
                }
            }
            ticketdelivery.dongleId = dongleIds;
        } else {
            ticketdelivery.dongleId = body.value("finishShellNo", ""); // 外壳号
        }
        ticketdelivery.remark = body.value("finishRemark", ""); // 备注
        ticketdelivery.executorId = (body.value("executorID", "")); // 执行人ID

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
        }));

    // 版本迭代+交付发送工单完成
    CROW_ROUTE(app, "/order/finish-iter-deliver").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        // 定义本地ticketpackage变量
        TicketPackage ticketpackage;
        
        // 检查字段
        ticketpackage.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketpackage.ticketType = "直接封装+发送"; // 工单类型 显示指定
        ticketpackage.status = "已完成"; // 更新工单状态为已完成
        ticketpackage.model = body.value("modelID", ""); // 关联模型
        ticketpackage.completedTime = body.value("finishTime", ""); // 完成时间
        ticketpackage.newModelVersion = body.value("finishModelVersion", ""); // 升级后模型版本ID
        ticketpackage.encrypted = (body.value("isEncrypted", "") == "是"); // 是否加密
        ticketpackage.license = body.value("finishAuthId", ""); // 授权ID
        // 处理authorizationIdList
        if (body.contains("authorizationId_list") && body["authorizationId_list"].is_array()) {
            std::string authIds;
            for (const auto& authId : body["authorizationId_list"]) {
                if (authId.is_string()) {
                    if (!authIds.empty()) {
                        authIds += ",";
                    }
                    authIds += authId.get<std::string>();
                }
            }
            ticketpackage.authorizationIdList = authIds;
        }
        // 处理外壳号列表
        if (body.contains("finishShellNo") && body["finishShellNo"].is_array()) {
            ticketpackage.dongle.clear();
            for (const auto& shellNo : body["finishShellNo"]) {
                if (shellNo.is_string()) {
                    ticketpackage.dongle.push_back(shellNo.get<std::string>());
                }
            }
        } else {
            ticketpackage.dongle.clear();
        }
        ticketpackage.remark = body.value("finishRemark", ""); // 备注
        ticketpackage.executorId = (body.value("executorID", "")); // 执行人ID

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
        }));

    // 功能开发工单完成
    CROW_ROUTE(app, "/order/finish-dev").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketFeature ticketfeature; // 定义本地ticketfeature变量

        // 检查字段
        ticketfeature.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketfeature.status = "已完成"; // 更新工单状态为已完成
        ticketfeature.ticketType = "功能开发"; // 工单类型 显示指定
        ticketfeature.model = body.value("modelID", ""); // 关联模型
        ticketfeature.completedTime = body.value("finishTime", ""); // 完成时间
        ticketfeature.newModelVersion = body.value("finishModelVersionId", ""); // 升级后模型版本ID
        ticketfeature.featureFinal = body.value("finishFeatureDesc", ""); // 完成功能描述
        ticketfeature.executorId = (body.value("executorID", "")); // 执行人ID

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
        }));

    // 其他工单完成
    CROW_ROUTE(app, "/order/finish-other").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketOther ticketother; // 定义本地ticketother变量

        // 检查字段
        ticketother.Ticket::id = std::stoi(body.value("orderID", "")); // 工单ID
        ticketother.status = "已完成"; // 更新工单状态为已完成
        ticketother.ticketType = "其他"; // 工单类型 显示指定
        ticketother.completedTime = std::stoi(body.value("finishTime", "")); // 完成时间
        ticketother.remark = body.value("finishRemarkOther", ""); // 完成备注
        ticketother.executorId = (body.value("executorID", "")); // 执行人ID

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
        }));

    // 工单流转
    CROW_ROUTE(app, "/order/transfer").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketTranfer ticketexecutor;

        // 清空并重新填充数组
        // ticketexecutor.executor.clear();
        // ticketexecutor.reason.clear();
        // ticketexecutor.timestamp.clear();

        // 检查字段
        ticketexecutor.ticketId = std::stoi(body.value("orderID", "")); // 工单ID
        ticketexecutor.createId.push_back(body.value("executorID", "")); // [0] 当前执行人ID
        ticketexecutor.executor.push_back(body.value("transferExecutorID", "")); // [1] 流转目标执行人ID
        ticketexecutor.reason.push_back(body.value("transferReason", "")); // [0] 流转原因
        ticketexecutor.timestamp.push_back(body.value("transferTime", "")); // [0] 流转时间
        ticketexecutor.ticketType = body.value("orderType", ""); // 工单类型
        ticketexecutor.transferType = body.value("transferType", ""); // 流转类型

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
        }));

    // 工单封装流转
    CROW_ROUTE(app, "/order/transfer_package").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketTranfer ticketexecutor;

        // 检查字段
        ticketexecutor.ticketId = std::stoi(body.value("orderID", "")); // 工单ID
        ticketexecutor.createId.push_back(body.value("executorID", "")); // 当前执行人ID
        ticketexecutor.executor.push_back(body.value("transferExecutorID", "")); // 流转目标执行人ID
        ticketexecutor.reason.push_back(body.value("transferReason", "")); // 流转原因
        ticketexecutor.timestamp.push_back(body.value("transferTime", "")); // 流转时间
        ticketexecutor.transferType = "封装流转"; // 固定为封装流转类型

        bool ok = ticketService->orderTransfer(ticketexecutor);
        
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "封装流转成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "封装流转失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        }));

    // 工单加密流转
    CROW_ROUTE(app, "/order/transfer_encrypted").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketTranfer ticketexecutor;

        // 检查字段
        ticketexecutor.ticketId = std::stoi(body.value("orderID", "")); // 工单ID
        ticketexecutor.createId.push_back(body.value("executorID", "")); // 当前执行人ID
        ticketexecutor.executor.push_back(body.value("transferExecutorID", "")); // 流转目标执行人ID
        ticketexecutor.reason.push_back(body.value("transferReason", "")); // 流转原因
        ticketexecutor.timestamp.push_back(body.value("transferTime", "")); // 流转时间
        ticketexecutor.transferType = "加密流转"; // 固定为加密流转类型
        ticketexecutor.newModelVersion = body.value("finishModelVersion", ""); // 完成模型版本

        bool ok = ticketService->orderTransfer(ticketexecutor);
        
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "加密流转成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "加密流转失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        }));

    // 工单发送流转
    CROW_ROUTE(app, "/order/transfer_send").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        TicketTranfer ticketexecutor;

        // 检查字段
        ticketexecutor.ticketId = std::stoi(body.value("orderID", "")); // 工单ID
        ticketexecutor.createId.push_back(body.value("executorID", "")); // 当前执行人ID
        ticketexecutor.executor.push_back(body.value("transferExecutorID", "")); // 流转目标执行人ID
        ticketexecutor.reason.push_back(body.value("transferReason", "")); // 流转原因
        ticketexecutor.timestamp.push_back(body.value("transferTime", "")); // 流转时间
        ticketexecutor.transferType = "发送流转"; // 固定为发送流转类型

        bool ok = ticketService->orderTransfer(ticketexecutor);
        
        nlohmann::json resp;
        if (ok) {
            resp = {
                {"status", 1},
                {"error", ""},
                {"data", {{"message", "发送流转成功"}}}
            };
        }
        else {
            resp = {
                {"status", 1},
                {"error", "发送流转失败"},
                {"data", nlohmann::json::object()}
            };
        }
        return crow::response{ resp.dump() };
        }));

    // 获取客户信息列表
    CROW_ROUTE(app, "/customer/list").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
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
        }));

    // // 添加新客户
    // CROW_ROUTE(app, "/customer/add").methods("POST"_method)
    //     (withAspect([this](const crow::request& req) {
    //     // // JWT校验
    //     // if (!checkToken(req)) {
    //     //     return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
    //     // }
    //     auto body = nlohmann::json::parse(req.body, nullptr, false);
    //     if (body.is_discarded() || !body.contains("name")) {
    //         return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
    //     }
    //     bool ok = service_.addCustomer(body["name"].get<std::string>());
    //     nlohmann::json resp;
    //     if (ok) {
    //         resp = {
    //             {"status", 1},
    //             {"error", ""},
    //             {"data", {{"message", "添加客户成功"}}}
    //         };
    //     }
    //     else {
    //         resp = {
    //             {"status", 1},
    //             {"error", "添加客户失败"},
    //             {"data", nlohmann::json::object()}
    //         };
    //     }
    //     return crow::response{ resp.dump() };
    //     }));


    // 工单附件文件下载接口
    CROW_ROUTE(app, "/files/ticket").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        // , int ticketId, const std::string& filename
        int ticketId = std::stoi(req.url_params.get("ticketId"));
        std::string filename = req.url_params.get("filename");
        std::string decodedFilename = url_decode(filename);
        LOG_INFO("文件下载请求 - ticketId:%d, filename:%s\n", ticketId, decodedFilename.c_str());
        return downloadTicketFile(ticketId, decodedFilename);
        }));


    CROW_ROUTE(app, "/order/statisticsAll").methods("GET"_method)
    ([this](const crow::request& req) {
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }

        auto params = crow::query_string(req.url_params);
        std::string clientNamesStr = params.get("clientNames") ? params.get("clientNames") : "";
        std::vector<std::string> clientNames;
        if (!clientNamesStr.empty()) {
            std::istringstream ss(clientNamesStr);
            std::string name;
            while (std::getline(ss, name, ',')) {
                if (!name.empty()) clientNames.push_back(name);
            }
        }

        // 查询所有统计数据
        // 0: 一周, 1: 一月, 2: 半年
        // 版本迭代
        auto dailyIter = ticketService->getOrderStatisticsByCondition(0, "版本迭代", clientNames);
        auto weeklyIter = ticketService->getOrderStatisticsByCondition(1, "版本迭代", clientNames);
        auto monthlyIter = ticketService->getOrderStatisticsByCondition(2, "版本迭代", clientNames);
        // 交付发送
        auto dailyDeliver = ticketService->getOrderStatisticsByCondition(0, "交付发送", clientNames);
        auto weeklyDeliver = ticketService->getOrderStatisticsByCondition(1, "交付发送", clientNames);
        auto monthlyDeliver = ticketService->getOrderStatisticsByCondition(2, "交付发送", clientNames);

        // 构造 daily
        nlohmann::json daily = nlohmann::json::array();
        for (size_t i = 0; i < clientNames.size(); ++i) {
            const std::string& clientName = clientNames[i];
            // 以日期为key聚合
            std::map<std::string, nlohmann::json> dateMap;
            for (const auto& pair : dailyIter[i]) {
                dateMap[pair.first]["date"] = pair.first;
                dateMap[pair.first]["clientName"] = clientName;
                dateMap[pair.first]["versionIterationCount"] = pair.second;
            }
            for (const auto& pair : dailyDeliver[i]) {
                dateMap[pair.first]["date"] = pair.first;
                dateMap[pair.first]["clientName"] = clientName;
                dateMap[pair.first]["deliveryCount"] = pair.second;
            }
            for (auto& kv : dateMap) {
                if (!kv.second.contains("versionIterationCount")) kv.second["versionIterationCount"] = 0;
                if (!kv.second.contains("deliveryCount")) kv.second["deliveryCount"] = 0;
                daily.push_back(kv.second);
            }
        }

        // 构造 weekly
        nlohmann::json weekly = nlohmann::json::array();
        for (size_t i = 0; i < clientNames.size(); ++i) {
            const std::string& clientName = clientNames[i];
            std::map<std::string, nlohmann::json> weekMap;
            for (const auto& pair : weeklyIter[i]) {
                weekMap[pair.first]["weekLabel"] = pair.first;
                weekMap[pair.first]["clientName"] = clientName;
                weekMap[pair.first]["versionIterationCount"] = pair.second;
            }
            for (const auto& pair : weeklyDeliver[i]) {
                weekMap[pair.first]["weekLabel"] = pair.first;
                weekMap[pair.first]["clientName"] = clientName;
                weekMap[pair.first]["deliveryCount"] = pair.second;
            }
            for (auto& kv : weekMap) {
                if (!kv.second.contains("versionIterationCount")) kv.second["versionIterationCount"] = 0;
                if (!kv.second.contains("deliveryCount")) kv.second["deliveryCount"] = 0;
                weekly.push_back(kv.second);
            }
        }

        // 构造 monthly
        nlohmann::json monthly = nlohmann::json::array();
        for (size_t i = 0; i < clientNames.size(); ++i) {
            const std::string& clientName = clientNames[i];
            std::map<std::string, nlohmann::json> monthMap;
            for (const auto& pair : monthlyIter[i]) {
                monthMap[pair.first]["monthLabel"] = pair.first;
                monthMap[pair.first]["clientName"] = clientName;
                monthMap[pair.first]["versionIterationCount"] = pair.second;
            }
            for (const auto& pair : monthlyDeliver[i]) {
                monthMap[pair.first]["monthLabel"] = pair.first;
                monthMap[pair.first]["clientName"] = clientName;
                monthMap[pair.first]["deliveryCount"] = pair.second;
            }
            for (auto& kv : monthMap) {
                if (!kv.second.contains("versionIterationCount")) kv.second["versionIterationCount"] = 0;
                if (!kv.second.contains("deliveryCount")) kv.second["deliveryCount"] = 0;
                monthly.push_back(kv.second);
            }
        }

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"daily", daily},
                {"weekly", weekly},
                {"monthly", monthly}
            }}
        };
        return crow::response{ resp.dump() };
    });

    // 新增复杂工单查询分页接口
    CROW_ROUTE(app, "/order/details").methods("GET"_method)
        ([this](const crow::request& req) {
            try {
                // 解析查询参数
                auto page_param = req.url_params.get("page");
                auto pageSize_param = req.url_params.get("pageSize");
                auto modelName_param = req.url_params.get("modelID");

                int page = page_param ? std::stoi(page_param) : 1;
                int pageSize = pageSize_param ? std::stoi(pageSize_param) : 10;
                std::string modelName = modelName_param ? std::string(modelName_param) : "ATA04_Aerodynamics";
                if(modelName == "ATA21_30_36_52_ECS"){
                    modelName = "ATA21/30/36/52_ECS";
                }
                // 计算偏移量
                int offset = (page - 1) * pageSize;
                
                // 获取分页的版本信息
                std::vector<std::pair<std::string, std::string>> versionInfos = ticketService->getVersionsWithPagination(modelName, offset, pageSize);
                
                // 获取总版本数
                unsigned long long totalVersions = ticketService->getVersionsCount(modelName);
                
                // 提取版本列表
                std::vector<std::string> versions;
                for (const auto& versionInfo : versionInfos) {
                    versions.push_back(versionInfo.first);
                }
                
                // 使用版本列表查询所有相关工单
                std::vector<nlohmann::json> allOrders = ticketService->getWorkOrdersWithDetailsByVersions(modelName, versions);
                
                // 按版本分组工单
                std::map<std::string, nlohmann::json> versionGroups;
                
                // 为每个版本创建分组
                for (const auto& versionInfo : versionInfos) {
                    const std::string& version = versionInfo.first;
                    const std::string& updateTime = versionInfo.second;
                    
                    nlohmann::json versionGroup;
                    versionGroup["version"] = version;
                    versionGroup["modelID"] = modelName;
                    versionGroup["updateTime"] = updateTime;
                    versionGroup["orders"] = nlohmann::json::array();
                    
                    versionGroups[version] = versionGroup;
                }
                
                // 将工单分配到对应的版本组
                for (const auto& order : allOrders) {
                    std::string orderVersion = order["version"];
                    if (versionGroups.find(orderVersion) != versionGroups.end()) {
                        versionGroups[orderVersion]["orders"].push_back(order);
                    }
                }
                
                // 构建返回的JSON结构
                nlohmann::json list = nlohmann::json::array();
                for (const auto& versionInfo : versionInfos) {

                    list.push_back(versionGroups[versionInfo.first]);
                }
                
                // 构建响应
                nlohmann::json resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {
                        {"list", list},
                        {"total", totalVersions}
                    }}
                };
                
                return crow::response(200, resp.dump());
            }
            catch (const std::exception& e) {
                
                nlohmann::json error = {
                    {"status", 1},
                    {"error", "查询工单失败"}
                };
                return crow::response(500, error.dump());
            }
        });

    // 获取用户待办工单（包含详细信息和流转信息）
    CROW_ROUTE(app, "/order/pending").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        
        try {
            // 从JWT中获取用户ID
            std::string userId = getAccountFromToken(req);
            // 调用服务层获取用户待办工单
            auto pendingOrders = ticketService->getUserPendingWorkOrders(userId);
            
            // 构建响应
            nlohmann::json resp = {
                {"status", 1},
                {"error", ""},
                {"data", {
                    {"list", pendingOrders},
                    {"total", pendingOrders.size()}
                }}
            };
            
            return crow::response(200, resp.dump());
        }
        catch (const std::exception& e) {
            nlohmann::json error = {
                {"status", 1},
                {"error", "获取用户待办工单失败"}
            };
            return crow::response(500, error.dump());
        }
    }));

    // 待封装工单完成接口
    CROW_ROUTE(app, "/order/finish-package").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        // 解析请求参数
        std::string orderID = body.value("orderID", "");
        std::string modelID = body.value("modelID", "");
        std::string orderType = body.value("orderType", "");
        std::string status = body.value("status", "");
        std::string finishTime = body.value("finishTime", "");
        std::string finishModelVersion = body.value("finishModelVersion", "");
        std::string packageRemark = body.value("packageRemark", "");
        std::string executorID = body.value("executorID", "");
        std::string encryptedExecutorId = body.value("encryptedExecutorID", "");

        // 参数验证
        if (orderID.empty() || orderType.empty()) {
            return crow::response(400, R"({"status":1,"error":"缺少必要参数","data":{}})");
        }

        try {
            bool result = false;
            
            // 如果是版本迭代工单，调用 completeConcreteTicket
            if (orderType == "版本迭代") {
                // 创建 TicketVersion 对象
                TicketVersion ticket;
                // 设置基类Ticket的id字段
                ticket.Ticket::id = std::stoi(orderID);
                // 设置派生类TicketVersion的id字段
                ticket.id = std::stoi(orderID);
                ticket.ticketType = orderType;
                ticket.model = modelID;
                ticket.newModelVersion = finishModelVersion;
                ticket.remark = packageRemark;
                ticket.completedTime = finishTime;
                ticket.status = status;
                ticket.executorId = executorID;
                
                result = ticketService->completeSendTicket(ticket);
            } else if(orderType == "版本迭代+交付发送"){
                // 版本迭代+交付发送类型的工单处理逻辑

                // 创建 TicketVersion 对象
                TicketPackage ticket;
                // 设置基类Ticket的id字段
                ticket.Ticket::id = std::stoi(orderID);
                // 设置派生类TicketVersion的id字段
                ticket.id = std::stoi(orderID);
                ticket.ticketType = orderType;
                ticket.model = modelID;
                ticket.newModelVersion = finishModelVersion;
                ticket.remark = packageRemark;
                ticket.completedTime = finishTime;
                ticket.status = status;
                ticket.executorId = executorID;
                ticket.encryptedExecutorId = encryptedExecutorId;

                result = ticketService->completePackageSendTicket(ticket);
            }else {
                return crow::response(400, R"({"status":1,"error":"暂不支持该工单类型的完成操作","data":{}})");
            }
            
            nlohmann::json resp;
            if (result) {
                resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {{"message", "工单完成成功"}}}
                };
            } else {
                resp = {
                    {"status", 1},
                    {"error", "工单完成失败"},
                    {"data", nlohmann::json::object()}
                };
            }
            
            return crow::response{resp.dump()};
        }
        catch (const std::exception& e) {
            nlohmann::json error = {
                {"status", 1},
                {"error", "处理请求时发生错误"}
            };
            return crow::response(500, error.dump());
        }
    }));

    // 工单加密接口
    CROW_ROUTE(app, "/order/finish-encrypt").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        // 解析请求参数
        std::string orderID = body.value("orderID", "");
        std::string status = body.value("status", "");
        std::string orderType = body.value("orderType", "");
        std::string finishTime = body.value("finishTime", "");
        std::string isEncrypted = body.value("isEncrypted", "");
        std::string finishAuthId = body.value("finishAuthId", "");
        std::string encryptedRemark = body.value("encryptedRemark", "");
        std::string executorID = body.value("executorID", "");
        std::string sendExecutorID = body.value("sendExecutorID", "");
        std::string encryptedExecutorID = body.value("executorID", "");

        // 参数验证
        if (orderID.empty() || orderType.empty()) {
            return crow::response(400, R"({"status":1,"error":"缺少必要参数","data":{}})");
        }

        try {
            bool result = false;
            
            // 如果是交付发送工单，调用 completeConcreteTicket
            if (orderType == "交付发送") {
                // 创建 TicketDelivery 对象
                TicketDelivery ticket;
                // 设置基类Ticket的id字段
                ticket.Ticket::id = std::stoi(orderID);
                // 设置派生类TicketDelivery的id字段
                ticket.id = std::stoi(orderID);
                ticket.ticketType = orderType;
                ticket.status = status;
                ticket.completedTime = finishTime;
                ticket.encrypted = (isEncrypted == "是");
                ticket.licenseId = finishAuthId;
                ticket.remark = encryptedRemark;
                ticket.executorId = executorID;
                ticket.sendExecutorId = sendExecutorID;
                ticket.encryptedExecutorId = encryptedExecutorID;
                
                // 处理外壳号列表
                if (body.contains("finishShellNo") && body["finishShellNo"].is_array()) {
                    std::string shellNos;
                    for (const auto& shellNo : body["finishShellNo"]) {
                        if (shellNo.is_string()) {
                            if (!shellNos.empty()) {
                                shellNos += ",";
                            }
                            shellNos += shellNo.get<std::string>();
                        }
                    }
                    ticket.dongleId = shellNos;
                }
                
                // 处理授权ID主键数组
                if (body.contains("authorizationId_list") && body["authorizationId_list"].is_array()) {
                    std::string authIds;
                    for (const auto& authId : body["authorizationId_list"]) {
                        if (authId.is_string()) {
                            if (!authIds.empty()) {
                                authIds += ",";
                            }
                            authIds += authId.get<std::string>();
                        }
                    }
                    ticket.authorizationIdList = authIds;
                }
                
                result = ticketService->completeConcreteTicket(ticket);
            } else if(orderType == "版本迭代+交付发送") {
                // 版本迭代+交付发送的工单处理逻辑
                TicketPackage ticket;
                // 设置基类Ticket的id字段
                ticket.Ticket::id = std::stoi(orderID);
                // 设置派生类TicketPackage的id字段
                ticket.id = std::stoi(orderID);
                ticket.ticketType = orderType;
                ticket.status = status;
                ticket.completedTime = finishTime;
                ticket.encrypted = (isEncrypted == "是");
                ticket.license = finishAuthId;
                ticket.remark = encryptedRemark;
                ticket.executorId = executorID;
                ticket.sendExecutorId = sendExecutorID;
                ticket.encryptedExecutorId = encryptedExecutorID;
                            
                // 处理外壳号列表
                if (body.contains("finishShellNo") && body["finishShellNo"].is_array()) {
                    std::string shellNos;
                    for (const auto& shellNo : body["finishShellNo"]) {
                        if (shellNo.is_string()) {
                            if (!shellNos.empty()) {
                                shellNos += ",";
                            }
                            shellNos += shellNo.get<std::string>();
                        }
                    }
                    ticket.dongles = shellNos;
                }
                
                result = ticketService->completePackageSendEncryptedTicket(ticket);
                
            } else {
                // 其他类型的工单处理逻辑
                return crow::response(400, R"({"status":1,"error":"暂不支持该工单类型的加密完成操作","data":{}})");
            }
            
            nlohmann::json resp;
            if (result) {
                resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {{"message", "交付发送工单加密完成成功"}}}
                };
            } else {
                resp = {
                    {"status", 1},
                    {"error", "交付发送工单加密完成失败"},
                    {"data", nlohmann::json::object()}
                };
            }
            
            return crow::response{resp.dump()};
        }
        catch (const std::exception& e) {
            nlohmann::json error = {
                {"status", 1},
                {"error", "处理请求时发生错误"}
            };
            return crow::response(500, error.dump());
        }
    }));

    // 待发送工单完成接口
    CROW_ROUTE(app, "/order/finish-send").methods("POST"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":1,"error":"无效token","data":{}})");
        }
        
        auto body = nlohmann::json::parse(req.body, nullptr, false);
        if (body.is_discarded()) {
            return crow::response(400, R"({"status":1,"error":"Invalid JSON","data":{}})");
        }

        // 解析请求参数
        std::string orderID = body.value("orderID", "");
        std::string status = body.value("status", "");
        std::string orderType = body.value("orderType", "");
        std::string finishTime = body.value("finishTime", "");
        std::string sendRemark = body.value("sendRemark", "");
        std::string executorID = body.value("executorID", "");
        std::string newModelVersion = body.value("finishVersion", "");
        // 参数验证
        if (orderID.empty() || orderType.empty() || executorID.empty()) {
            return crow::response(400, R"({"status":1,"error":"缺少必要参数","data":{}})");
        }

        try {
            bool result = false;
            
            // 处理交付发送工单
            if (orderType == "交付发送") {
                TicketDelivery ticket;
                // 设置基类Ticket的id字段
                ticket.Ticket::id = std::stoi(orderID);
                // 设置派生类TicketDelivery的id字段
                ticket.id = std::stoi(orderID);
                ticket.ticketType = orderType;
                ticket.status = "已完成"; // 发送完成后状态为已完成
                ticket.completedTime = finishTime;
                ticket.remark = sendRemark;
                ticket.executorId = executorID;
                ticket.sendExecutorId = executorID; // 发送人ID
                
                result = ticketService->completeSendTicket(ticket);
            }
            // 处理直接封装+发送工单
            else if (orderType == "版本迭代+交付发送") {
                TicketPackage ticket;
                // 设置基类Ticket的id字段
                ticket.Ticket::id = std::stoi(orderID);
                // 设置派生类TicketPackage的id字段
                ticket.id = std::stoi(orderID);
                ticket.ticketType = orderType;
                ticket.status = "已完成"; // 发送完成后状态为已完成
                ticket.completedTime = finishTime;
                ticket.remark = sendRemark;
                ticket.executorId = executorID;
                ticket.sendExecutorId = executorID; // 发送人ID
                ticket.newModelVersion = newModelVersion;
                
                result = ticketService->completeSendTicket(ticket);
            }
            else {
                return crow::response(400, R"({"status":1,"error":"不支持的工单类型","data":{}})");
            }
            
            nlohmann::json resp;
            if (result) {
                resp = {
                    {"status", 1},
                    {"error", ""},
                    {"data", {{"message", "工单发送完成成功"}}}
                };
            } else {
                resp = {
                    {"status", 1},
                    {"error", "工单发送完成失败"},
                    {"data", nlohmann::json::object()}
                };
            }
            
            return crow::response{resp.dump()};
        }
        catch (const std::exception& e) {
            nlohmann::json error = {
                {"status", 1},
                {"error", "处理请求时发生错误"}
            };
            return crow::response(500, error.dump());
        }
    }));
}
