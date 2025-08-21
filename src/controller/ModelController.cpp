#include "ModelController.h"
#include "jwt_utils.h"
#include "Log.h"

// 匿名命名空间 - 仅在当前文件可见
namespace {
    int safeStoi(const std::string& str, int defaultValue = 0) {
        if (str.empty() || str == "null" || str == "undefined") {
            return defaultValue;
        }
        try {
            return std::stoi(str);
        } catch (const std::exception& e) {
            printf("[WARNING] safeStoi failed for '%s': %s, using default %d\n", 
                   str.c_str(), e.what(), defaultValue);
            return defaultValue;
        }
    }
    
    std::string safeGetParam(const char* param) {
        return param ? std::string(param) : std::string("");
    }
}

ModelController::ModelController(std::shared_ptr<IModelService> sp) : modelService(sp) {}

void ModelController::registerRoutes(crow::App<crow::CORSHandler>& app) {
    // 获取模型列表
    CROW_ROUTE(app, "/model/list").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 解析分页参数 - 使用安全转换
        auto params = crow::query_string(req.url_params);
        
        // 安全获取参数
        std::string pageStr = safeGetParam(params.get("page"));
        std::string pageSizeStr = safeGetParam(params.get("pageSize"));
        std::string modelID = safeGetParam(params.get("modelID"));
        
        // 安全转换为整数
        int page = safeStoi(pageStr.empty() ? "1" : pageStr, 1);
        int pageSize = safeStoi(pageSizeStr.empty() ? "10" : pageSizeStr, 10);
        
        // 参数验证
        if (page <= 0) page = 1;
        if (pageSize <= 0 || pageSize > 100) pageSize = 10;

        // 获取模型列表及工单信息，支持分页
        std::vector<std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Ticket>>>> modelListWithTickets = 
            modelService->getModelVersionWithOrdersByModelPaged(modelID, page-1, pageSize);

        // 转换为目标JSON格式
        nlohmann::json modelList = nlohmann::json::array();
        
        for (const auto& modelVersionPair : modelListWithTickets) {
            nlohmann::json modelJson;
            
            // 解析模型版本信息 [模型名, 版本号, 创建时间]
            const auto& versionInfo = modelVersionPair.first;
            if (versionInfo.size() >= 3) {
                modelJson["modelID"] = versionInfo[0];     // 模型名
                modelJson["version"] = versionInfo[1];     // 版本号
                modelJson["updatetime"] = versionInfo[2];  // 创建时间
            }
            
            // 转换工单信息 - 直接使用实体类的序列化方法
            nlohmann::json ordersJson = nlohmann::json::array();
            const auto& tickets = modelVersionPair.second;
            
            for (const auto& ticket : tickets) {
                if (ticket) {
                    // 使用多态的 to_json() 方法
                    nlohmann::json orderJson = ticket->to_json();
                    ordersJson.push_back(orderJson);
                }
            }
            
            modelJson["orders"] = ordersJson;
            modelList.push_back(modelJson);
        }
        
        // 计算总数
        int total = modelService->getModelVersionCount(modelID);      
        printf("total: %d\n", total);  
        
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", modelList},
                {"total", total}
            }}
        };
        
        return crow::response{ resp.dump() };
        }));

    // 获取模型版本列表
    CROW_ROUTE(app, "/model/version/list").methods("GET"_method)
        (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        auto params = crow::query_string(req.url_params);
        std::string modelId = params.get("modelId") ? params.get("modelId") : "";

        std::vector<std::string> modelVersions;

        modelVersions = modelService->getModelVersionByModel(modelId);
        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", modelVersions}
            }}
        };
        return crow::response{ resp.dump() };
        }));

    // 根据创建时填写的部分版本号获取当前数据库中已有的模型版本
    CROW_ROUTE(app, "/model/version/used").methods("GET"_method)
    (withAspect([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }

        auto params = crow::query_string(req.url_params);
        std::string modelId = params.get("modelId") ? params.get("modelId") : "";
        std::string completeModelVersion = params.get("completeModelVersion") ? params.get("completeModelVersion") : "";

        if (modelId.empty() || completeModelVersion.empty()) {
            nlohmann::json resp = {
                {"status", 1},
                {"error", "缺少必要参数：modelId 或 completeModelVersion"},
                {"data", nlohmann::json::object()}
            };
            return crow::response(400, resp.dump());
        }

        // 调用 service 层方法
        std::vector<std::string> versions = modelService->getModelVersionByPartialModelVersion(modelId, completeModelVersion);

        nlohmann::json resp = {
            {"status", 1},
            {"error", ""},
            {"data", {
                {"list", versions}
            }}
        };
        return crow::response{ resp.dump() };
    }));
}