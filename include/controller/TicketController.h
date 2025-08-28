/*****************************************************************//**
 * \file   TicketController.h
 * \brief  工单控制器，负责注册工单相关的 HTTP 路由
 * 
 * \author 
 * \date   July 2025
 *********************************************************************/
#pragma once
#include "ITicketService.h"
#include "entity/Entity.h"
#include "util/ticket.h"
#include <memory>
#include <crow.h>
#include "crow/middlewares/cors.h"
#include <nlohmann/json.hpp>

class TicketController
{
public:
    /**
     * @brief 构造函数，初始化 TicketController。
     */
    TicketController(std::shared_ptr<ITicketService>);

    /**
     * @brief 注册所有工单相关的路由到 Crow 应用。
     * @param app Crow 应用实例
     */
    void registerRoutes(crow::App<crow::CORSHandler>& app);

    /**
     * @struct MultipartResult
     * @brief multipart/form-data 解析结果结构体,专用于文件附件
     */
    MultipartResult parseMultipartForm(const std::string& content_type, const std::string& body);

    // 文件下载接口
    crow::response downloadTicketFile(int ticketId, const std::string& filename);
    
private:
    std::shared_ptr<ITicketService> ticketService; // 工单服务实例

    std::vector<std::string> customerList_; // 客户列表
    std::vector<std::string> shellNumberList_; // 外壳号列表
    std::vector<std::string> authIDList_; // 授权ID列表

    // 工单主结构体，包含所有工单类型的字段

};