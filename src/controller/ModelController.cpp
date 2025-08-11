#include "ModelController.h"
#include "jwt_utils.h"

ModelController::ModelController(std::shared_ptr<IModelService> sp) : modelService(sp) {}

void ModelController::registerRoutes(crow::SimpleApp& app) {
    // 获取模型列表
    CROW_ROUTE(app, "/model/list").methods("GET"_method)
        ([this](const crow::request& req) {
        // JWT校验
        if (!checkToken(req)) {
            return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
        }
        // 解析分页参数
        auto params = crow::query_string(req.url_params);
        int page = std::stoi(params.get("page") ? params.get("page") : "1");
        int pageSize = std::stoi(params.get("pageSize") ? params.get("pageSize") : "10");
        std::string modelID = params.get("modelID") ? params.get("modelID") : "";

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
        
        // 计算总数（这里简化处理，实际应该调用专门的计数方法）
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
        });

    // 获取模型版本列表
    CROW_ROUTE(app, "/model/version/list").methods("GET"_method)
        ([this](const crow::request& req) {
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
        });
}