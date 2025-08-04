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
    void registerRoutes(crow::SimpleApp& app);

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
	std::vector<std::string> approverList_; // 审批人列表
    std::vector<std::string> distributorList_; // 分发人列表
    std::vector<std::string> transferExecutorList_; // 执行人列表
    std::vector<std::string> executorList_; // 执行人列表

    Ticket ticket; // 工单主结构体，包含所有工单类型的字段
	TicketReproduce ticketreproduce; // 问题复现工单结构体
    TicketVersion ticketversion; // 版本迭代工单结构体
	TicketPackage ticketpackage; // 直接封装+发送工单结构体
	TicketDelivery ticketdelivery; // 交付发送工单结构体
	TicketFeature ticketfeature; // 功能开发工单结构体
	TicketOther ticketother; // 其他工单结构体

	TicketExecutor ticketexecutor; // 工单执行人结构体，用于记录流转信息
};