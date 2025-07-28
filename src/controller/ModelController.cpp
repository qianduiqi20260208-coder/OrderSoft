#include "ModelController.h"
#include "jwt_utils.h"

ModelController::ModelController(std::shared_ptr<IModelService> sp) : modelService(sp) {}

void ModelController::registerRoutes(crow::SimpleApp& app) {
    // // 获取模型列表
    // CROW_ROUTE(app, "/model/list").methods("GET"_method)
    //     ([this](const crow::request& req) {
    //     // // JWT校验
    //     // if (!checkToken(req)) {
    //     //     return crow::response(401, R"({"status":0,"error":"无效token","data":{}})");
    //     // }
    //     // 解析分页参数
    //     auto params = crow::query_string(req.url_params);
    //     int page = std::stoi(params.get("page") ? params.get("page") : "1");
    //     int pageSize = std::stoi(params.get("pageSize") ? params.get("pageSize") : "10");
    //     std::string modelID = params.get("modelID") ? params.get("modelID") : "";


    //     // 获取模型列表及工单信息，支持分页
    //     nlohmann::json models = service_.getModelList(page, pageSize, modelID);
    //     nlohmann::json resp = {
    //         {"status", 1},
    //         {"error", ""},
    //         {"data", models}
    //     };
    //     return crow::response{ resp.dump() };
    //     });

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