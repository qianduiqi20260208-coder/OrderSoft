#include "TicketDAO.h"
#include "IniReader.h"
#include <fstream>
#include <sstream>
#include <memory>
#include "DBConnectionManager.h"
#include "UserDAO.h"
#include <set>
#include "util/Logger.h"

TicketDAO::TicketDAO() : mysql(nullptr)
{
    // 使用连接池，不需要初始化mysql指针
}

TicketDAO::TicketDAO(MYSQL *ms):mysql(ms)
{
    LOG_WARNING("TicketDAO: 使用已废弃的构造函数，建议使用连接池");
    DBConnectionManager::getConnection(mysql);
}

bool TicketDAO::completeConcreteTicket(const Ticket &ticket)
{
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    if(ticket.ticketType == "问题复现")
    {
        const TicketReproduce& tmp = dynamic_cast<const TicketReproduce&>(ticket);

        snprintf(local_sql, SQL_MAX, "update issue_reproduction set phenomenon = '%s',remarks = '%s' where work_order_id = %d;", tmp.phenomenon.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改issue_reproduction表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "版本迭代")
    {
        const TicketVersion& tmp = dynamic_cast<const TicketVersion&>(ticket);

        //根据模型以及模型版本去模型版本表里把具体的模型版本ID查出来
        snprintf(local_sql, SQL_MAX, "update version_iteration set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),remarks = '%s' where work_order_id = %d;",tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改version_iteration表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        
        // 插入 work_order_executor 记录
        snprintf(local_sql, SQL_MAX, "INSERT INTO work_order_executor (work_order_id,executor_id, create_at, status, update_version,update_remark, create_id, finish_at) VALUES (%d,'%s', NOW(), '完成', '%s', '%s','%s', NOW());"
            , tmp.Ticket::id, tmp.executorId.c_str(), tmp.newModelVersion.c_str(), tmp.remark.c_str(), tmp.executorId.c_str());
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 插入work_order_executor表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else if(ticket.ticketType == "交付发送"){
        // //去产品授权表里根据外壳号以及授权码找出唯一的产品授权ID
        const TicketDelivery& tmp = dynamic_cast<const TicketDelivery&>(ticket);
        //判断是否加密，然后分开处理
        if(tmp.encrypted)
        {
            snprintf(local_sql, SQL_MAX, "update delivery_send set is_encrypted = %d,shell_code = '%s',remarks = '%s' ,auth_id = '%s' where work_order_id = %d;", 
                tmp.encrypted,tmp.dongleId.c_str(),tmp.remark.c_str(),tmp.licenseId.c_str(),tmp.Ticket::id);
        }else{
            snprintf(local_sql, SQL_MAX, "update delivery_send set is_encrypted = 0, remarks = '%s' where work_order_id = %d;", tmp.remark.c_str(), tmp.Ticket::id);
        }
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改delivery_send表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        
        snprintf(local_sql, SQL_MAX, "update work_order set encrypted_remark = '%s' where work_order_id = %d;", 
            tmp.remark.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改work_order表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        // INSERT INTO `work_order_executor` ( `work_order_id`, `executor_id`, `create_at`, `status`, `create_id`, `encryption_status`) 
        snprintf(local_sql, SQL_MAX, "INSERT INTO work_order_executor (work_order_id,executor_id, create_at, status, create_id, encryption_status) VALUES (%d,'%s', NOW(), '加密', '%s', '0');"
            , tmp.Ticket::id, tmp.sendExecutorId.c_str(), tmp.executorId.c_str());
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 插入work_order_executor表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        
    }else if(ticket.ticketType == "直接封装+发送"){
        
        const TicketPackage& tmp = dynamic_cast<const TicketPackage&>(ticket);
        if(tmp.encrypted)
        {
            std::string dongleStr = "";
            if (!tmp.dongle.empty()) {
                for (size_t i = 0; i < tmp.dongle.size(); ++i) {
                    if (i > 0) dongleStr += ",";
                    dongleStr += tmp.dongle[i];
                }
            }
            snprintf(local_sql, SQL_MAX, "update package_send set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),is_encrypted = %d,encryption_key = '%s' ,remarks = '%s',auth_id = '%s' where work_order_id = %d;", 
                tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.encrypted,dongleStr.c_str(),tmp.remark.c_str(),tmp.license.c_str(),tmp.Ticket::id);
        }else{
            snprintf(local_sql, SQL_MAX, "update package_send set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),is_encrypted = 0 where work_order_id = %d;", tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.Ticket::id);
        }

        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改package_send表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "功能开发"){
        const TicketFeature& tmp = dynamic_cast<const TicketFeature&>(ticket);
        snprintf(local_sql, SQL_MAX, "update function_development set description_completed = '%s',new_model_version_id = (select id from model_version where model = '%s' and version = '%s' ) where work_order_id = %d;",tmp.featureFinal.c_str(),tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        printf("sql:%s\n",local_sql);
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改function_development表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "其他"){
        const TicketOther& tmp = dynamic_cast<const TicketOther&>(ticket);
        snprintf(local_sql, SQL_MAX, "update other_work_order set remarks = '%s' where work_order_id = %d;",tmp.remark.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改other_work_order表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }

    //删除自己在流程中的位置
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '执行人' ;",ticket.id,ticket.executorId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:completeConcreteTicket 删除user_multi_role表失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        std::cerr << "事务提交失败：" << mysql_store_result(conn) << "\n";
        return false;
    }

    return true;
}

bool TicketDAO::saveUploadFile(const TicketReproduce &ticket)
{
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    //保存文件
    IniReader config;
    if (!config.load("config.ini")) {
            std::cerr << "无法读取 config.ini 文件\n";
        return false;
    }

    if(ticket.attachment.fileName != "")
    {
        std::string relativePath = "/ticket/"+ticket.attachment.fileName;
        std::string filePath = config.getString("storage","upload_dir") + relativePath;
        std::fstream ofs(filePath,std::ios::binary | std::ios::out);
        if(!ofs.is_open())
        {
            LOG_ERROR("function:saveUploadFile 文件打开失败!filePath:%s\n",filePath.c_str());
            ofs.close();
            return false;
        }else{
            ofs.write(ticket.attachment.file.c_str(),ticket.attachment.file.size());
            ofs.close();

            //数据库存储
            snprintf(local_sql, SQL_MAX, "INSERT INTO issue_reproduction_attachment(ticket_id,file_path,file_name) "
                "VALUES(%d,'%s', '%s');", ticket.Ticket::id, relativePath.c_str(),ticket.attachment.fileName.c_str());	
            local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
            if (local_ret) {
                LOG_ERROR("function:saveUploadFile 插入附件信息表失败！失败原因：%s", mysql_error(conn));
                mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
                return false;
            }
        }
    }

    return true;
}



bool TicketDAO::createTicket(Ticket &ticket)
{


        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    
    //启用事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:createTicket 事务开始失败！失败原因：%s", mysql_error(conn));
        return false;
    }
    //获取模型版本ID
    snprintf(local_sql, SQL_MAX, "select id from model_version where model = '%s' and version = '%s';", ticket.model.c_str(),ticket.modelVersion.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
		 LOG_ERROR("function:createTicket 查询model_version表失败！失败原因：%s", mysql_error(conn));
		return false;
	}
    local_res = mysql_store_result(conn);
    local_row = mysql_fetch_row(local_res);
    if(!local_row)
        return false;
    else
        ticket.modelVersion = local_row[0];
    // 释放结果集
	mysql_free_result(local_res);

    //往工单表中插入数据
	snprintf(local_sql, SQL_MAX, "INSERT INTO work_order(id,creator_id,type,model,model_version_id,approver_id) "
        "VALUES(NULL,'%s', '%s', '%s', %d, '%s');", ticket.creatorId.c_str(), ticket.ticketType.c_str(),ticket.model.c_str(),stoi(ticket.modelVersion), ticket.approverId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    printf("sql:%s\n",local_sql);
	if (local_ret) {
		 LOG_ERROR("function:createTicket 插入work_order表数据失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //记录另一个人的待办
    //获取主键id
    ticket.id = mysql_insert_id(conn);
    snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role values(NULL,'%s','null','审批人',%d);", ticket.approverId.c_str(),ticket.id);	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));

	if (local_ret) {
		 LOG_ERROR("function:createTicket 插入user_multi_role数据失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //往具体工单表中插入数据
    //判断是什么类型的工单
    if(ticket.ticketType == "问题复现")// 问题复现创建
    {
        TicketReproduce& tp = dynamic_cast<TicketReproduce&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO issue_reproduction(work_order_id,coordination_id,description) "
        "VALUES(%d,'%s','%s');", ticket.Ticket::id,tp.coordinationId.c_str(), tp.content.c_str());
        
        //这里需要先执行一次 避免sql被覆盖掉
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        LOG_DEBUG("SQL执行: %s", local_sql);
        if (local_ret) {
            LOG_ERROR("function:createConcreteTicket 插入issue_reproduction失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        if(!saveUploadFile(tp))
            return false;
    }else if(ticket.ticketType == "版本迭代")
    {
        //借用一个具体工单表里的remark字段 暂时保存前三位的版本号
        TicketVersion& tv = dynamic_cast<TicketVersion&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO version_iteration(work_order_id,coordination_id,update_content,packaging_requirements,interface_changed,matlab_version) "
        "VALUES(%d,'%s','%s', '%s',%d,'%s');",ticket.Ticket::id,tv.coordinationId.c_str(), tv.updateNote.c_str(),tv.packRequirement.c_str(),tv.interfaceChanged,tv.matlab_version.c_str());
    }else if(ticket.ticketType == "交付发送")
    {
        TicketDelivery& td = dynamic_cast<TicketDelivery&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO delivery_send(work_order_id,target_customer,validated_by_cae,sensitive_info) "
        "VALUES(%d,'%s',%d, '%s');", ticket.Ticket::id,td.targetClient.c_str(), td.validatedByCAE,td.sensitiveInfo.c_str());
    }else if(ticket.ticketType == "直接封装+发送")
    {
        TicketPackage& tp = dynamic_cast<TicketPackage&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO package_send(work_order_id,coordination_id,update_content,packaging_requirements,interface_changed,target_customer,validated_by_cae,sensitive_info,matlab_version) "
        "VALUES(%d,'%s','%s','%s',%d,'%s',%d,'%s','%s');", ticket.Ticket::id,tp.coordinationId.c_str(), tp.updateNote.c_str(),tp.packRequirement.c_str(),tp.interfaceChanged,tp.targetClient.c_str(),tp.validatedByCAE,tp.sensitiveInfo.c_str(),tp.matlab_version.c_str());        
    }else if(ticket.ticketType == "功能开发")
    {
        TicketFeature& tf = dynamic_cast<TicketFeature&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO function_development(work_order_id,description_create,matlab_version) "
        "VALUES(%d,'%s','%s');", ticket.Ticket::id,tf.featureInit.c_str(),tf.matlab_version.c_str());
    }else if(ticket.ticketType == "其他")
    {
        TicketOther& to = dynamic_cast<TicketOther&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO other_work_order(work_order_id,description) "
        "VALUES(%d,'%s');", ticket.Ticket::id,to.description.c_str());
    }
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));

	if (local_ret) {
		LOG_ERROR("function:createConcreteTicket 插入具体的工单表失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}
    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        LOG_ERROR("function:createConcreteTicket 事务提交失败！失败原因：%s", mysql_error(conn));
        return false;
    }
    return true;
}
bool TicketDAO::approveTicket(const Ticket &ticket)
{

        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    //启用事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:approveTicket 事务开始失败！失败原因：%s", mysql_error(conn));
        return false;
    }

    //修改工单状态
    //看优先级变量里是否有数据，没有数据表示拒绝，拒绝的话需要填写拒绝原因
    if(ticket.priorityHint == "")
    {
        snprintf(local_sql, SQL_MAX, "update work_order set status = '已退回',reject_reason = '%s' where id = %d;", ticket.rejectReason.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:createTicket 修改work_order表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else{
        snprintf(local_sql, SQL_MAX, "update work_order set status = '待分发', approved_at = NOW(),priority = '%s',dispatcher_id = '%s' where id = %d;", ticket.priorityHint.c_str(), ticket.distributorId.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:createTicket 修改work_order表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        
        // 如果ticket.targetDeliveryTime不为空，按照ticket.id更新delivery_send表的targetDeliveryTime
        if(ticket.targetDeliveryTime != "")
        {
            if(ticket.ticketType == "交付发送"){

                snprintf(local_sql, SQL_MAX, "update delivery_send set target_delivery_time = '%s' where work_order_id = %d;", ticket.targetDeliveryTime.c_str(),ticket.id);
            }else if (ticket.ticketType == "版本迭代+交付发送")
            {
                snprintf(local_sql, SQL_MAX, "update package_send set target_delivery_time = '%s' where work_order_id = %d;", ticket.targetDeliveryTime.c_str(),ticket.id);
            }
            local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
            if (local_ret) {
                LOG_ERROR("function:createTicket 修改delivery_send表失败！失败原因：%s", mysql_error(conn));
                mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
                return false;
            }
        }
        LOG_INFO("function:approveTicket 更新delivery_send表的targetDeliveryTime:%s", ticket.targetDeliveryTime.c_str());
        //记录另一个人的待办
        snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values('%s','分发人',%d);", ticket.distributorId.c_str(),ticket.id);	
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:createTicket 插入user_multi_role数据失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }

    //删除自己在流程中的位置
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '审批人' ;",ticket.id, ticket.approverId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:createTicket 删除user_multi_role数据失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        LOG_ERROR("function:approveTicket 事务提交失败!失败原因：%s", mysql_error(conn));
        return false;
    }

    return true;
}
bool TicketDAO::dispatchTicket(const Ticket& ticket, const std::string& account)
{

        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    //启用事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:dispatchTicket 事务开始失败！失败原因：%s", mysql_error(conn));
        return false;
    }

    //看任务优先级变量里是否有数据，没有数据表示拒绝，拒绝的话需要填写拒绝原因
    if(ticket.priorityTask == "")
    {
        snprintf(local_sql, SQL_MAX, "update work_order set status = '已退回',dispatcher_reject_reason = '%s' where id = %d;", ticket.rejectReason.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 修改work_order表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else{
        //修改工单状态
        if(ticket.ticketType == "问题复现" || ticket.ticketType == "功能开发" || ticket.ticketType == "其他")
        {
            snprintf(local_sql, SQL_MAX, "update work_order set status = '进行中', status_todo = '待完成',dispatched_at = NOW(),task_priority = '%s', to_dispatcher_id = '%s' where id = %d;", ticket.priorityTask.c_str(), ticket.executorId.c_str(), ticket.id);
        }
        else if(ticket.ticketType == "版本迭代" || ticket.ticketType == "版本迭代+交付发送")
        {
            snprintf(local_sql, SQL_MAX, "update work_order set status = '进行中', status_todo = '待封装',dispatched_at = NOW(),task_priority = '%s', to_dispatcher_id = '%s' where id = %d;", ticket.priorityTask.c_str(), ticket.executorId.c_str(), ticket.id);
        }else if(ticket.ticketType == "交付发送"){
            snprintf(local_sql, SQL_MAX, "update work_order set status = '进行中', status_todo = '待加密',dispatched_at = NOW(),task_priority = '%s', to_dispatcher_id = '%s' where id = %d;", ticket.priorityTask.c_str(), ticket.executorId.c_str(), ticket.id);
        }

        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 修改work_order表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

        //记录工单执行人 ---
        // 分发-- 只是封装
        snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`,  `status`, `create_id`) VALUES (%d, '%s', NOW(), '封装', '%s');", ticket.id,ticket.executorId.c_str(),account.c_str());
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 修改work_order_executor表失败!失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

        //记录另一个人的待办
        snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values('%s','执行人',%d);",ticket.executorId.c_str(),ticket.id);	
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 插入user_multi_role表失败！executorId失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }

    //删除自己的待办
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '分发人' ;",ticket.id, ticket.distributorId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:dispatchTicket 删除user_multi_role表失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
		 LOG_ERROR("function:dispatchTicket 事务提交失败!失败原因：%s", mysql_error(conn));
        return false;
    }
    return true;
}
bool TicketDAO::completeTicket(const Ticket &ticket)
{

        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
        //启用事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:completeTicket 事务开始失败！失败原因：%s", mysql_error(conn));
        return false;
    }


    //修改工单状态
    if(ticket.ticketType == "交付发送"){
        // 加密操作
        auto& ticketDelivery = dynamic_cast<const TicketDelivery&>(ticket);
        // 先根据是否加密判断状态 ticketDelivery.remark.c_str()可以为空
        if(ticketDelivery.encrypted){
            // 修改delivery_send加密信息
            snprintf(local_sql, SQL_MAX, "update `delivery_send` set `is_encrypted` = %d, `shell_code` = '%s', `remarks` = '%s', `auth_id` = '%s' where `work_order_id` = %d;",
            ticketDelivery.encrypted ? 1 : 0, ticketDelivery.dongleId.c_str(), ticketDelivery.remark.c_str(), ticketDelivery.licenseId.c_str(), ticket.id);
            //执行SQL
            if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                std::cerr << "更新delivery_send加密信息失败：" << mysql_error(conn) << std::endl;
                mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                return false;
            }
            
            // 插入work_order_executor 加密
            snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`,  `status`, `create_id`,product_authorization_remark) VALUES (%d, '%s', NOW(), '加密', '%s','%s');",
                ticket.id,ticket.executorId.c_str(),ticket.executorId.c_str(),ticketDelivery.remark.c_str());
            if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                std::cerr << "插入work_order_executor加密记录失败：" << mysql_error(conn) << std::endl;
                mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                return false;
            }
            
            // 如果sendExecutorId 和 加密的executorId 不同 修改user_multi_role
            if(ticket.executorId != ticket.sendExecutorId){
                snprintf(local_sql, SQL_MAX, "update user_multi_role set user_id = '%s' where work_order_id = %d and user_id = '%s' and flow_role = '执行人';",
                ticket.sendExecutorId.c_str(),ticket.id,ticket.executorId.c_str());
                if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                    std::cerr << "更新user_multi_role失败：" << mysql_error(conn) << std::endl;
                    mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                    return false;
                }
                
                // 插入work_order_executor 发送流转
                // snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`,  `status`, `create_id`,transfer_type) VALUES (%d, '%s', NOW(), '流转', '%s', '发送流转');",
                // ticket.id,ticket.sendExecutorId.c_str(),ticket.executorId.c_str());
                // if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                //     std::cerr << "插入work_order_executor发送流转记录失败：" << mysql_error(conn) << std::endl;
                //     mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                //     return false;
                // }
            }else{
                // 插入work_order_executor 发送
                snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`,  `status`, `create_id`) VALUES (%d, '%s', NOW(), '发送', '%s');",
                ticket.id,ticket.sendExecutorId.c_str(),ticket.executorId.c_str());
                if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                    std::cerr << "插入work_order_executor发送记录失败：" << mysql_error(conn) << std::endl;
                    mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                    return false;
                }
            }
            
        }else{
            // 不用加密
            snprintf(local_sql, SQL_MAX, "update `delivery_send` set `is_encrypted` = %d,  `remarks` = '%s' where `work_order_id` = %d;",
            ticketDelivery.encrypted ? 1 : 0, ticketDelivery.remark.c_str(), ticket.id);
            if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                std::cerr << "更新delivery_send信息失败：" << mysql_error(conn) << std::endl;
                mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                return false;
            }
            // 添加没有加密的执行记录
            
            snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`,  `status`, `create_id`) VALUES (%d, '%s', NOW(), '加密', '%s');",
            ticket.id,ticket.executorId.c_str(),ticket.executorId.c_str());
            if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
                std::cerr << "插入work_order_executor加密记录失败：" << mysql_error(conn) << std::endl;
                mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                return false;
            }

            
        }
        
        snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`,  `status`, `create_id`) VALUES (%d, '%s', NOW(), '发送', '%s');",
        ticket.id,ticket.sendExecutorId.c_str(),ticket.executorId.c_str());
        if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
            std::cerr << "插入work_order_executor发送记录失败：" << mysql_error(conn) << std::endl;
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        // 修改work_order status_todo信息
        snprintf(local_sql, SQL_MAX, "update work_order set status_todo = '待发送',encrypted_remark = '%s' where id = %d;",ticketDelivery.remark.c_str(),ticket.id);
        if (mysql_real_query(conn, local_sql, strlen(local_sql))) {
            std::cerr << "更新work_order状态失败：" << mysql_error(conn) << std::endl;
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
            std::cerr << "交付发送事务提交失败：" << mysql_store_result(conn) << "\n";
            return false;
        }
        return true;
    }else if(ticket.ticketType == "版本迭代+交付发送"){
        snprintf(local_sql, SQL_MAX, "update work_order set status_todo = '待封装' where id = %d;",ticket.id);
    }else {
        snprintf(local_sql, SQL_MAX, "update work_order set status = '已完成',completed_at = now() where id = %d;",ticket.id);
    }
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
		 LOG_ERROR("function:completeTicket  修改work_order表失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    return completeConcreteTicket(ticket);
}

std::vector<std::shared_ptr<Ticket>> TicketDAO::selectOrderByCondition_(
    const std::map<std::string, std::string>& filter, 
    int offset, 
    int count)
{
    
    if (!ensureConnection()) {
         return {std::shared_ptr<Ticket>()};
    }
    
    MYSQL* conn = getConnection();
    
    std::vector<std::shared_ptr<Ticket>> retVec;

    std::stringstream ss;
    ss<<"select distinct wo.* from work_order wo left join delivery_send ds on ds.work_order_id = wo.id"
    " left join package_send ps on ps.work_order_id = wo.id"
    " join work_order_executor woe on woe.work_order_id = wo.id";
    bool first = true;
    //根据类型的不同进行不同的处理
    std::set<std::string> intSet={"id","creator_id","model_version_id","approver_id","dispatcher_id","executor_id"};

    if(!filter.empty())
    {
        ss<<" where "; //如果没有过滤条件，则查询所有
    }

    for(auto ele: filter)
    {
        if(!first) ss<<" and ";

        //首先判断字段是否是int类型
        if(intSet.find(ele.first) !=intSet.end())
        {
            if(ele.first == "executor_id")
            {
                ss<<"woe."<<ele.first<<" = "<<ele.second;
            }else if(ele.first == "id")
            {
                ss<<"wo."<<ele.first<<" = "<<ele.second;
            }
            else{
                ss << ele.first << " = " << ele.second;
            }

        }else{
            //不是int类型的情况下判断传入的是否是模型数组
            if(ele.first == "model" && ele.second.find(' ')!= std::string::npos)
            {
                //使用iss来分割字符串
                std::istringstream iss(ele.second);
                std::string token;
                std::vector<std::string> values;
                while (iss >> token) {
                    values.push_back(token);
                }
                ss << ele.first << " IN (";
                for (size_t i = 0; i < values.size(); ++i) {
                    ss << values[i];
                    if (i != values.size() - 1) ss << ",";
                }
                ss << ")";
            }else if(ele.first == "target_delivery_time"){
                //预计发送时间单独处理 因为涉及到别的表了
                ss<<"(DATE(ds."<<ele.first<<") = '"<<ele.second<<"' or ";
                ss<<"DATE(ps."<<ele.first<<") = '"<<ele.second<<"')";
            }else if(ele.first == "status")
            {
                ss<<"wo."<<ele.first<<" = '"<<ele.second<<"'";
            }else if(ele.first == "completed_at" && ele.second == "NOT NULL")
            {
                //处理查询完成时间为空的工单
                ss<<"wo."<<ele.first<<" IS NOT NULL";
            }
            else{
                ss<<ele.first<<" = '"<<ele.second<<"'";
            }
        }

        first = false;
    }

    ss<<" order by wo.id desc limit "<<offset<<","<<count<<";";


    int local_ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    

    LOG_ERROR("function:selectOrderByCondition() sql：%s", ss.str().c_str());
    if (local_ret) {
        LOG_ERROR("function:selectOrderByCondition() 查询work_order表失败！失败原因：%s", mysql_error(conn));

        return {std::shared_ptr<Ticket>()};
    }
    MYSQL_RES* local_res = mysql_store_result(conn);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        std::shared_ptr<Ticket> tmp;
        //判断工单的类型
        if(std::string(local_row[3]) == "问题复现")
        {
            tmp = std::make_shared<TicketReproduce>();
        }else if(std::string(local_row[3]) == "版本迭代")
        {   
            tmp = std::make_shared<TicketVersion>();
        }else if(std::string(local_row[3]) == "直接封装+发送")
        {
            tmp = std::make_shared<TicketPackage>();
        }else if(std::string(local_row[3]) == "交付发送")
        {
            tmp = std::make_shared<TicketDelivery>();
        }else if(std::string(local_row[3]) == "功能开发")
        {
            tmp = std::make_shared<TicketFeature>();
        }else if(std::string(local_row[3]) == "其他")
        {
            tmp = std::make_shared<TicketOther>();
        }
        extern std::map<int,std::string> id_name;
        //封装一些共有的信息
        tmp->id = atoi(local_row[0]);
        tmp->creatorId = id_name[atoi(local_row[1])];
        tmp->createTime = local_row[2];
        tmp->ticketType = local_row[3];
        tmp->model = local_row[4];
        tmp->modelVersion = local_row[5];
        tmp->status = local_row[6];
        tmp->approverId = id_name[atoi(local_row[7])];

        tmp->priorityHint = (local_row[8]?local_row[8]:"");
        tmp->distributorId = id_name[atoi(local_row[9])];
        tmp->approvedTime = local_row[10]?local_row[10]:"";
        tmp->priorityTask = local_row[11]?local_row[11]:"";
        tmp->distributedTime = local_row[12]?local_row[12]:"";
        tmp->completedTime = local_row[13]?local_row[13]:"";
        tmp->rejectReason = local_row[14]?local_row[14]:"";
        tmp->rejectReason = local_row[14]?local_row[14]:"";
        tmp->dispatchRejectReason = local_row[15]?local_row[15]:"";

        //查询模型版本
        if(tmp->modelVersion !="")
        {
            tmp->modelVersion = UserDAO::queryModelVersion(stoi(tmp->modelVersion));
        }
        //封装工单执行人
        tmp->executor = UserDAO::queryTicketExecutor(tmp->id);
        //封装一些私有的信息
        concreteTicketList(tmp->id,tmp,retVec);
    }
    
    return retVec;
}



void TicketDAO::concreteTicketList(int work_order_id, std::shared_ptr<Ticket> vecElement,std::vector<std::shared_ptr<Ticket>> & retVec)
{
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    //判断具体工单的类型
    if(vecElement->ticketType == "问题复现")
    {
        auto tmp = std::static_pointer_cast<TicketReproduce>(vecElement);
        // 保存ticketId，用于查询附件
        tmp->ticketId = work_order_id;
        snprintf(local_sql, SQL_MAX, "select * from issue_reproduction where work_order_id = %d;",tmp->Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:concreteTicketList() 查询issue_reproduction表失败！失败原因：%s", mysql_error(conn));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->coordinationId = (row[2]?row[2]:"");
            tmp->content = (row[3]?row[3]:"");
            tmp->phenomenon = (row[5]?row[5]:"");
            tmp->remark = (row[6]?row[6]:"");
        }
        mysql_free_result(res);

        //查询附件的信息(文件上传)
        UserDAO::downloadAttachment(tmp);
        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "版本迭代"){

        auto tmp = std::static_pointer_cast<TicketVersion>(vecElement);
        snprintf(local_sql, SQL_MAX, "select * from version_iteration where work_order_id = %d;",tmp->Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:concreteTicketList() 查询version_iteration表失败！失败原因：%s", mysql_error(conn));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->coordinationId = (row[2]?row[2]:"");
            tmp->updateNote = (row[3]?row[3]:"");
            tmp->packRequirement = (row[4]?row[4]:"");
            tmp->interfaceChanged = atoi((row[5]?row[5]:"-1"));
            tmp->newModelVersion = row[6]?row[6]:"";
            tmp->remark = (row[7]?row[7]:"");
        }
        mysql_free_result(res);
        // todo
        if(tmp->newModelVersion !="")
        {
            int modelVersionId = stoi(tmp->newModelVersion);
            tmp->newModelVersion = UserDAO::queryModelVersion(modelVersionId);
            //查询基模型版本
            tmp->baseModelVersion = UserDAO::queryBaseModelVersion(modelVersionId);
        }


        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "直接封装+发送"){
        auto tmp = std::static_pointer_cast<TicketPackage>(vecElement);

        snprintf(local_sql, SQL_MAX, "select * from package_send where work_order_id = %d;",tmp->Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:concreteTicketList() 查询package_send表失败！失败原因：%s", mysql_error(conn));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->coordinationId = (row[2]?row[2]:"");
            tmp->updateNote = (row[3]?row[3]:"");
            tmp->packRequirement = (row[4]?row[4]:"");
            tmp->interfaceChanged = atoi((row[5]?row[5]:"-1"));
            tmp->targetClient = (row[6]?row[6]:"");
            tmp->validatedByCAE = atoi((row[7]?row[7]:"-1"));
            tmp->sensitiveInfo = (row[8]?row[8]:"");

            tmp->newModelVersion = row[9]?row[9]:"";
            tmp->encrypted = atoi((row[10]?row[10]:"-1"));
            tmp->dongle.clear();
            if (row[11] && strlen(row[11]) > 0) {
                tmp->dongle.push_back(row[11]);
            }
            tmp->remark = (row[12]?row[12]:"");
            tmp->targetDeliveryTime = (row[13]?row[13]:"");
            tmp->license =  row[15]?row[15]:"";

        }
        mysql_free_result(res);

        if(tmp->newModelVersion !="")
        {
            int modelVersionId = stoi(tmp->newModelVersion);
            tmp->newModelVersion = UserDAO::queryModelVersion(modelVersionId);
            //查询基模型版本
            tmp->baseModelVersion = UserDAO::queryBaseModelVersion(modelVersionId);
        }

        if(tmp->license != "")
            tmp->license = UserDAO::queryProductAuthorization(stoi(tmp->license));
        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "交付发送"){
        auto tmp = std::static_pointer_cast<TicketDelivery>(vecElement);
        snprintf(local_sql, SQL_MAX, "select * from delivery_send where work_order_id = %d;",tmp->Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:concreteTicketList() 查询delivery_send表失败！失败原因：%s", mysql_error(conn));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->targetClient = (row[2]?row[2]:"");
            tmp->validatedByCAE = atoi((row[3]?row[3]:"-1"));
            tmp->sensitiveInfo = (row[4]?row[4]:"");

            tmp->encrypted = atoi((row[5]?row[5]:"-1"));
            tmp->dongleId =  (row[6]?row[6]:"");

            tmp->remark = (row[7]?row[7]:"");
            tmp->targetDeliveryTime = (row[8]?row[8]:"");
            tmp->licenseId =  (row[9]?row[9]:"");
        }
        //查找产品授权ID的sql
        if(tmp->licenseId != "")
            tmp->licenseId = UserDAO::queryProductAuthorization(stoi(tmp->licenseId));

        mysql_free_result(res);
        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "功能开发"){
        auto tmp = std::static_pointer_cast<TicketFeature>(vecElement);
        snprintf(local_sql, SQL_MAX, "select * from function_development where work_order_id = %d;",tmp->Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:concreteTicketList() 查询function_development表失败！失败原因：%s", mysql_error(conn));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->featureInit = (row[2]?row[2]:"");
            tmp->featureFinal = (row[3]?row[3]:"");
            tmp->newModelVersion = ((row[5])?row[5]:"");
        }
        //查询模型版本
        if(tmp->newModelVersion !="")
        {
            int modelVersionId = stoi(tmp->newModelVersion);
            tmp->newModelVersion = UserDAO::queryModelVersion(modelVersionId);
            //查询基模型版本
            tmp->baseModelVersion = UserDAO::queryBaseModelVersion(modelVersionId);
        }

        mysql_free_result(res);
        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "其他"){
        auto tmp = std::static_pointer_cast<TicketOther>(vecElement);
        snprintf(local_sql, SQL_MAX, "select * from other_work_order where work_order_id = %d;",tmp->Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:concreteTicketList() 查询other_work_order表失败！失败原因：%s", mysql_error(conn));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->description= (row[2]?row[2]:"");
            tmp->remark = (row[3]?row[3]:"");
        }
        mysql_free_result(res);
        retVec.push_back(tmp);
    }

}
// 工单流转
bool TicketDAO::orderTransfer(const TicketTranfer &executor)
{

        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;

    //启用事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:orderTransfer 事务开始失败！失败原因：%s", mysql_error(conn));
        return false;
    }
    //流转 状态未发送
    snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`, `transfer_reason`, `status`, `create_id`, `encryption_status`, `transfer_type`, `update_version`) VALUES (%d, %d, NOW(), '%s', '流转', %d, '0', '%s', '%s');"
        , executor.ticketId, stoi(executor.executor[0]), executor.reason[0].c_str(), stoi(executor.createId[0]), executor.transferType.c_str(), executor.newModelVersion.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:orderTransfer 添加work_order_executor表失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
        return false;
    }
    // work_order_executor如果工单是封转流转需要插入一条封装记录,如果是加密流转要插入一条加密记录,如果是发送流转要插入一条发送记录 
    // 定义一个字符串变量
    std::string transferType = "流转"; // 默认值为"流转"
    if(executor.transferType == "封转流转"){
        transferType ="封装";
    } else if(executor.transferType == "加密流转"){
        transferType ="加密";
    } else if(executor.transferType == "发送流转"){
        transferType="发送";
    }
    
    // 验证transferType是否为有效的枚举值
    if(transferType != "封装" && transferType != "加密" && transferType != "流转" && transferType != "发送") {
        LOG_ERROR("function:orderTransfer 无效的流转类型: %s", transferType.c_str());
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
        return false;
    }
    snprintf(local_sql, SQL_MAX, "INSERT INTO `work_order_executor` (`work_order_id`, `executor_id`, `create_at`, `transfer_reason`, `status`, `create_id`, `encryption_status`,  `update_version`) VALUES (%d, %d, NOW(), '%s', '%s', %d, '0', '%s');"
        , executor.ticketId, stoi(executor.executor[0]), executor.reason[0].c_str(),transferType.c_str(), stoi(executor.createId[0]), executor.newModelVersion.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        // 打印sql
        printf("sql:%s\n",local_sql);
        LOG_ERROR("function:orderTransfer 添加work_order_executor表失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
        return false;
    }
    

    //记录另一个人的待办
    snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values(%d,'执行人',%d);",stoi(executor.executor[0]),executor.ticketId);	
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    printf("sql:%s\n",local_sql);
    if (local_ret) {
        LOG_ERROR("function:orderTransfer 插入user_multi_role数据失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
        return false;
    }
    
    //删除自己在流程中的位置
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d and flow_role = '执行人' ;",executor.ticketId,stoi(executor.executor[0]));	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:orderTransfer 删除user_multi_role数据失败！失败原因：%s", mysql_error(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        LOG_ERROR("function:orderTransfer 事务提交失败！失败原因：%s", mysql_error(conn));
        return false;
    }

    return true;
}

unsigned long long TicketDAO::getOrderCount(const std::map<std::string, std::string>& filter)
{
    if (!ensureConnection()) {
        return 0;
    }
    MYSQL* conn = getConnection();
    std::stringstream ss;
    ss << "SELECT COUNT(*) FROM work_order ";
    bool first = true;
    std::set<std::string> intSet = {"id","creator_id","model_version_id","approver_id","dispatcher_id"};

    if (!filter.empty()) {
        ss << "WHERE ";
    }

    for (const auto& ele : filter) {
        if (!first) ss << " AND ";
        if (intSet.find(ele.first) != intSet.end()) {
            if (ele.first == "model" && ele.second.find(' ') != std::string::npos) {
                std::istringstream iss(ele.second);
                std::string token;
                std::vector<std::string> values;
                while (iss >> token) {
                    values.push_back(token);
                }
                ss << ele.first << " IN (";
                for (size_t i = 0; i < values.size(); ++i) {
                    ss << values[i];
                    if (i != values.size() - 1) ss << ",";
                }
                ss << ")";
            } else {
                ss << ele.first << " = " << ele.second;
            }
        } else {
            if (ele.first == "model" && ele.second.find(' ') != std::string::npos) {
                std::istringstream iss(ele.second);
                std::string token;
                std::vector<std::string> values;
                while (iss >> token) {
                    values.push_back(token);
                }
                ss << ele.first << " IN (";
                for (size_t i = 0; i < values.size(); ++i) {
                    ss << "'" << values[i] << "'";
                    if (i != values.size() - 1) ss << ",";
                }
                ss << ")";
            } else {
                if (ele.second.find(',') != std::string::npos) {
                    std::istringstream iss(ele.second);
                    std::string token;
                    std::vector<std::string> values;
                    while (std::getline(iss, token, ',')) {
                        token.erase(0, token.find_first_not_of(" \t"));
                        token.erase(token.find_last_not_of(" \t") + 1);
                        if (!token.empty()) {
                            values.push_back(token);
                        }
                    }
                    ss << ele.first << " IN (";
                    for (size_t i = 0; i < values.size(); ++i) {
                        ss << "'" << values[i] << "'";
                        if (i != values.size() - 1) ss << ",";
                    }
                    ss << ")";
                } else {
                    ss << ele.first << " = '" << ele.second << "'";
                }
            }
        }
        first = false;
    }

    ss << ";";
    int local_ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    if (local_ret) {
        LOG_ERROR("function:getOrderCount() 查询work_order表失败！失败原因：%s", mysql_error(conn));
        return 0;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    unsigned long long retCount = 0;
    if ((row = mysql_fetch_row(res))) {
        retCount = row[0] ? std::stoull(row[0]) : 0;
    }
    mysql_free_result(res);
    return retCount;
}

std::vector<std::string> TicketDAO::getOrderClient()
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
         return {"select fail!"};
    }
    
    MYSQL* conn = getConnection();
    std::vector<std::string> retVec;

    char local_sql[SQL_MAX];
    int local_ret;
    
    snprintf(local_sql, SQL_MAX, "select customer_name from customer_info;");
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderClient() 查询customer_info表失败！失败原因：%s", mysql_error(conn));
        return  {"select fail!"};
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    while(row = mysql_fetch_row(res))
    {
        retVec.push_back(std::string(row[0]));
    }
    mysql_free_result(res);

    return retVec;
}



std::vector<nlohmann::json> TicketDAO::getVersionsWithPagination(const std::string& modelName, int offset, int pageSize)
{

    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
         return {};
    }
    
    MYSQL* conn = getConnection();
    
    std::vector<nlohmann::json> retVec;
    
    // 构建版本分页查询SQL
    std::stringstream ss;
    ss << "SELECT version, update_time as updateTime "
       << "FROM model_version "
       << "WHERE model = '" << modelName << "' "
       << "AND version IS NOT NULL "
       << "ORDER BY id DESC "
       << "LIMIT " << offset << ", " << pageSize << ";";
    
    int local_ret;
    local_ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    
    if (local_ret) {
        LOG_ERROR("function:getVersionsWithPagination() 查询失败！失败原因：%s", mysql_error(conn));
        return {};
    }
    
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    while(row = mysql_fetch_row(res))
    {
        nlohmann::json versionJson;
        versionJson["version"] = row[0] ? std::string(row[0]) : "";
        versionJson["updateTime"] = row[1] ? std::string(row[1]) : "";
        retVec.push_back(versionJson);
    }
    
    mysql_free_result(res);
    return retVec;
}

unsigned long long TicketDAO::getVersionsCount(const std::string& modelName)
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
         return 0;
    }
    
    MYSQL* conn = getConnection();
    
    // 构建版本计数查询SQL
    std::stringstream ss;
    ss << "SELECT COUNT(DISTINCT version) "
       << "FROM model_version "
       << "WHERE model = '" << modelName << "' "
       << "AND version IS NOT NULL;";
    int local_ret;

    local_ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    
    if (local_ret) {
        LOG_ERROR("function:getVersionsCount() 查询失败！失败原因：%s", mysql_error(conn));
        return 0;
    }
    
    MYSQL_RES* res;
    res = mysql_store_result(conn);
    unsigned long long count = 0;
    MYSQL_ROW row;
    if(row = mysql_fetch_row(res))
    {
        count = row[0] ? std::stoull(row[0]) : 0;
    }
    
    mysql_free_result(res);
    return count;
}

std::vector<nlohmann::json> TicketDAO::getWorkOrdersWithDetailsByVersions(const std::string& modelName, const std::vector<std::string>& versions)
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return {};
    }
    
    MYSQL* conn = getConnection();
    
    if(versions.empty())
    {
        return {};
    }
    
    std::vector<nlohmann::json> retVec;
    
    // 构建版本IN子句
    std::stringstream versionInClause;
    versionInClause << "(";
    for(size_t i = 0; i < versions.size(); ++i)
    {
        if(i > 0) versionInClause << ", ";
        versionInClause << "'" << versions[i] << "'";
    }
    versionInClause << ")";
    
    // 构建复杂的SQL查询，使用UNION来正确处理版本迭代工单
    std::stringstream ss;
    ss << "("
       << "SELECT DISTINCT "
       << "wo.id, "
       << "mv.id as mv_id, "
       << "mv.version, "
       << "mv.update_time, "
       << "u1.real_name AS creatorId, "
       << "wo.created_at as createTime, "
       << "wo.type as ticketType, "
       << "wo.model, "
       << "(SELECT version FROM model_version WHERE id = wo.model_version_id) as modelVersion, "
       << "wo.status, "
       << "u2.real_name  AS approverId, "
       << "wo.priority as priorityHint, "
       << "u3.real_name AS distributorId, "
       << "u4.real_name AS executorID, "
       << "wo.approved_at as approvedTime, "
       << "wo.task_priority as priorityTask, "
       << "wo.dispatched_at as distributedTime, "
       << "wo.completed_at as completedTime, "
       << "wo.reject_reason as rejectReason, "
       << "CASE "
       << "  WHEN wo.type = '问题复现' THEN ir.coordination_id "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.coordination_id "
       << "  ELSE NULL "
       << "END AS coordinationId, "
       << "CASE "
       << "  WHEN wo.type = '问题复现' THEN ir.description "
       << "  ELSE NULL "
       << "END AS content, "
       << "CASE "
       << "  WHEN wo.type = '其他' THEN owo.description "
       << "  ELSE NULL "
       << "END AS description, "
       << "CASE "
       << "  WHEN wo.type = '问题复现' THEN ir.phenomenon "
       << "  ELSE NULL "
       << "END AS phenomenon, "
       << "CASE "
       << "  WHEN wo.type = '问题复现' THEN ir.remarks "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.remarks "
       << "  WHEN wo.type = '交付发送' THEN ds.remarks "
       << "  WHEN wo.type = '其他' THEN owo.remarks "
       << "  ELSE NULL "
       << "END AS remark, "
       << "CASE "
       << "  WHEN wo.type = '问题复现' THEN ira.file_name "
       << "  ELSE NULL "
       << "END AS fileName, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.update_content "
       << "  ELSE NULL "
       << "END AS updateNote, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.packaging_requirements "
       << "  ELSE NULL "
       << "END AS packRequirement, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.interface_changed "
       << "  ELSE NULL "
       << "END AS interfaceChanged, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.new_model_version_id "
       << "  WHEN wo.type = '功能开发' THEN fd.new_model_version_id "
       << "  ELSE NULL "
       << "END AS newModelVersion, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.target_customer "
       << "  WHEN wo.type = '交付发送' THEN ds.target_customer "
       << "  ELSE NULL "
       << "END AS targetClient, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.validated_by_cae "
       << "  WHEN wo.type = '交付发送' THEN ds.validated_by_cae "
       << "  ELSE NULL "
       << "END AS validatedByCae, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.sensitive_info "
       << "  WHEN wo.type = '交付发送' THEN ds.sensitive_info "
       << "  ELSE NULL "
       << "END AS sensitiveInfo, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.is_encrypted "
       << "  WHEN wo.type = '交付发送' THEN ds.is_encrypted "
       << "  ELSE NULL "
       << "END AS encrypted, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.encryption_key "
       << "  ELSE NULL "
       << "END AS dongle, "
       << "CASE "
       << "  WHEN wo.type = '交付发送' THEN ds.shell_code "
       << "  ELSE NULL "
       << "END AS dongleId, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.auth_id "
       << "  ELSE NULL "
       << "END AS license, "
       << "CASE "
       << "  WHEN wo.type = '交付发送' THEN ds.auth_id"
       << "  ELSE NULL "
       << "END AS licenseId, "
       << "CASE "
       << "  WHEN wo.type = '功能开发' THEN fd.description_create "
       << "  ELSE NULL "
       << "END AS featureInit, "
       << "CASE "
       << "  WHEN wo.type = '功能开发' THEN fd.description_completed "
       << "  ELSE NULL "
       << "END AS featureFinal "
       << "FROM model_version mv "
       << "LEFT JOIN work_order wo ON mv.id = wo.model_version_id "
       << "LEFT JOIN (SELECT woe1.* FROM work_order_executor woe1 INNER JOIN (SELECT work_order_id, MAX(id) as max_id FROM work_order_executor GROUP BY work_order_id) woe2 ON woe1.work_order_id = woe2.work_order_id AND woe1.id = woe2.max_id) woe ON woe.work_order_id = wo.id "
       << "LEFT JOIN issue_reproduction ir ON ir.work_order_id = wo.id "
       << "LEFT JOIN issue_reproduction_attachment ira ON ira.ticket_id = ir.work_order_id "
       << "LEFT JOIN package_send ps ON ps.work_order_id = wo.id "
       << "LEFT JOIN delivery_send ds ON ds.work_order_id = wo.id "
       << "LEFT JOIN function_development fd ON fd.work_order_id = wo.id "
       << "LEFT JOIN other_work_order owo ON owo.work_order_id = wo.id "
       << "LEFT JOIN user u1 ON u1.username = wo.creator_id "
       << "LEFT JOIN user u2 ON u2.username = wo.approver_id "
       << "LEFT JOIN user u3 ON u3.username = wo.dispatcher_id "
       << "LEFT JOIN user u4 ON u4.username = woe.executor_id "
       << "WHERE mv.model = '" << modelName << "' "
       << "AND mv.version IS NOT NULL "
       << "AND mv.version IN " << versionInClause.str() << " "
       << "AND wo.type != '版本迭代' AND MV.version = (CASE WHEN wo.type = '功能开发' THEN (SELECT version FROM model_version WHERE id = fd.new_model_version_id) WHEN wo.type = '直接封装+发送' THEN (SELECT version FROM model_version WHERE id = ps.new_model_version_id) ELSE NULL  END)  and wo.completed_at is not null"
       << ") UNION ("
       << "SELECT DISTINCT "
       << "wo.id, "
       << "mv.id as mv_id, "
       << "mv.version, "
       << "mv.update_time, "
       << "u1.real_name AS creatorId, "
       << "wo.created_at as createTime, "
       << "wo.type as ticketType, "
       << "wo.model, "
       << "(SELECT version FROM model_version WHERE id = wo.model_version_id) as modelVersion, "
       << "wo.status, "
       << "u2.real_name  AS approverId, "
       << "wo.priority as priorityHint, "
       << "u3.real_name AS distributorId, "
       << "u4.real_name AS executorID, "
       << "wo.approved_at as approvedTime, "
       << "wo.task_priority as priorityTask, "
       << "wo.dispatched_at as distributedTime, "
       << "wo.completed_at as completedTime, "
       << "wo.reject_reason as rejectReason, "
       << "CASE "
       << "  WHEN wo.type = '版本迭代' THEN vi.coordination_id "
       << "  WHEN wo.type = '功能开发' THEN NULL "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.coordination_id "
       << "  ELSE NULL "
       << "END AS coordinationId, "
       << "NULL AS content, "
       << "CASE "
       << "  WHEN wo.type = '功能开发' THEN fd.description_create "
       << "  ELSE NULL "
       << "END AS description, "
       << "NULL AS phenomenon, "
       << "CASE "
       << "  WHEN wo.type = '版本迭代' THEN vi.remarks "
       << "  WHEN wo.type = '功能开发' THEN fd.description_completed "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.remarks "
       << "  ELSE NULL "
       << "END as remark, "
       << "NULL AS fileName, "
       << "CASE "
       << "  WHEN wo.type = '版本迭代' THEN vi.update_content "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.update_content "
       << "  ELSE NULL "
       << "END AS updateNote, "
       << "CASE "
       << "  WHEN wo.type = '版本迭代' THEN vi.packaging_requirements "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.packaging_requirements "
       << "  ELSE NULL "
       << "END AS packRequirement, "
       << "CASE "
       << "  WHEN wo.type = '版本迭代' THEN vi.interface_changed "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.interface_changed "
       << "  ELSE NULL "
       << "END AS interfaceChanged, "
       << "CASE "
       << "  WHEN wo.type = '版本迭代' THEN (SELECT version FROM model_version WHERE id = vi.new_model_version_id) "
       << "  WHEN wo.type = '功能开发' THEN (SELECT version FROM model_version WHERE id = fd.new_model_version_id) "
       << "  WHEN wo.type = '直接封装+发送' THEN (SELECT version FROM model_version WHERE id = ps.new_model_version_id) "
       << "  ELSE NULL "
       << "END AS newModelVersion, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.target_customer "
       << "  ELSE NULL "
       << "END AS targetClient, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.validated_by_cae "
       << "  ELSE NULL "
       << "END AS validatedByCae, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.sensitive_info "
       << "  ELSE NULL "
       << "END AS sensitiveInfo, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.is_encrypted "
       << "  ELSE NULL "
       << "END AS encrypted, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.encryption_key "
       << "  ELSE NULL "
       << "END AS dongle, "
       << "NULL AS dongleId, "
       << "CASE "
       << "  WHEN wo.type = '直接封装+发送' THEN ps.auth_id "
       << "  ELSE NULL "
       << "END AS license, "
       << "NULL AS licenseId, "
       << "CASE "
       << "  WHEN wo.type = '功能开发' THEN fd.description_create "
       << "  ELSE NULL "
       << "END AS featureInit, "
       << "CASE "
       << "  WHEN wo.type = '功能开发' THEN fd.description_completed "
       << "  ELSE NULL "
       << "END AS featureFinal "
       << "FROM model_version mv "
       << "INNER JOIN ("
       << "  SELECT vi.new_model_version_id as target_version_id, vi.work_order_id, '版本迭代' as work_type FROM version_iteration vi "
       << "  UNION ALL "
       << "  SELECT fd.new_model_version_id as target_version_id, fd.work_order_id, '功能开发' as work_type FROM function_development fd "
       << "  UNION ALL "
       << "  SELECT ps.new_model_version_id as target_version_id, ps.work_order_id, '直接封装+发送' as work_type FROM package_send ps "
       << ") target_works ON mv.id = target_works.target_version_id "
       << "INNER JOIN work_order wo ON wo.id = target_works.work_order_id "
       << "LEFT JOIN version_iteration vi ON vi.work_order_id = wo.id AND wo.type = '版本迭代' "
       << "LEFT JOIN function_development fd ON fd.work_order_id = wo.id AND wo.type = '功能开发' "
       << "LEFT JOIN package_send ps ON ps.work_order_id = wo.id AND wo.type = '直接封装+发送' "
       << "LEFT JOIN (SELECT woe1.* FROM work_order_executor woe1 INNER JOIN (SELECT work_order_id, MAX(id) as max_id FROM work_order_executor GROUP BY work_order_id) woe2 ON woe1.work_order_id = woe2.work_order_id AND woe1.id = woe2.max_id) woe ON woe.work_order_id = wo.id "
       << "LEFT JOIN user u1 ON u1.username = wo.creator_id "
       << "LEFT JOIN user u2 ON u2.username = wo.approver_id "
       << "LEFT JOIN user u3 ON u3.username = wo.dispatcher_id "
       << "LEFT JOIN user u4 ON u4.username = woe.executor_id "
       << "WHERE mv.model = '" << modelName << "' "
       << "AND mv.version IS NOT NULL "
       << "AND mv.version IN " << versionInClause.str() << " "
       << "AND wo.type IN ('版本迭代', '功能开发', '直接封装+发送')  and wo.completed_at is not null"
       << ") ORDER BY id DESC;";

    int ret;
    ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    
    if (ret) {
        LOG_ERROR("function:getWorkOrdersWithDetailsByVersions() 查询失败！失败原因：%s", mysql_error(conn));
        return {};
    }
    MYSQL_RES* res;
    MYSQL_ROW row;
    res = mysql_store_result(conn);
    while(row = mysql_fetch_row(res))
    {
        nlohmann::json orderJson;
        
        // 基本字段（与原方法相同的字段映射逻辑）
        orderJson["orderID"] = row[0] ? std::string(row[0]) : "";
        orderJson["mv_id"] = row[1] ? std::string(row[1]) : "";
        orderJson["version"] = row[2] ? std::string(row[2]) : "";
        orderJson["update_time"] = row[3] ? std::string(row[3]) : "";
        orderJson["creatorId"] = row[4] ? std::string(row[4]) : "";
        orderJson["promoterID"] = row[4] ? std::string(row[4]) : ""; // 发起人ID
        orderJson["startTime"] = row[5] ? std::string(row[5]) : ""; // 发起时间
        orderJson["createTime"] = row[5] ? std::string(row[5]) : "";
        orderJson["ticketType"] = row[6] ? std::string(row[6]) : "";
        orderJson["model"] = row[7] ? std::string(row[7]) : "";
        orderJson["modelVersion"] = row[8] ? std::string(row[8]) : "";
        orderJson["status"] = row[9] ? std::string(row[9]) : "";
        orderJson["approverID"] = row[10] ? std::string(row[10]) : "";
        orderJson["priorityHint"] = row[11] ? std::string(row[11]) : "";
        orderJson["referencePriority"] = row[11] ? std::string(row[11]) : ""; // 参考优先级
        orderJson["distributorID"] = row[12] ? std::string(row[12]) : "";
        orderJson["executorID"] = row[13] ? std::string(row[13]) : "";
        orderJson["approvedTime"] = row[14] ? std::string(row[14]) : "";
        orderJson["priorityTask"] = row[15] ? std::string(row[15]) : "";
        orderJson["distributedTime"] = row[16] ? std::string(row[16]) : "";
        orderJson["distributeTime"] = row[16] ? std::string(row[16]) : ""; // 分发时间
        orderJson["completedTime"] = row[17] ? std::string(row[17]) : "";
        orderJson["rejectReason"] = row[18] ? std::string(row[18]) : "";
        
        // 条件字段
        orderJson["coordinationID"] = row[19] ? std::string(row[19]) : "";
        orderJson["content"] = row[20] ? std::string(row[20]) : "";
        orderJson["description"] = row[21] ? std::string(row[21]) : "";
        orderJson["phenomenon"] = row[22] ? std::string(row[22]) : "";
        orderJson["finishPhenomenon"] = row[22] ? std::string(row[22]) : ""; // 复现现象
        orderJson["remark"] = row[23] ? std::string(row[23]) : "";
        std::string fileName = row[24] ? std::string(row[24]) : "";
        orderJson["fileName"] = fileName;
        
        // 根据Entity.h中的逻辑添加fileUrl和hasAttachment
        if (!fileName.empty()) {
            orderJson["fileUrl"] = "/files/ticket/" + (row[0] ? std::string(row[0]) : "") + "/" + fileName;
            orderJson["hasAttachment"] = true;
        } else {
            orderJson["hasAttachment"] = false;
        }
        
        orderJson["updateNote"] = row[25] ? std::string(row[25]) : "";
        orderJson["packRequirement"] = row[26] ? std::string(row[26]) : "";
        orderJson["interfaceChanged"] = row[27] ? std::string(row[27]) : "";
        orderJson["newModelVersion"] = row[28] ? std::string(row[28]) : "";
        orderJson["targetClient"] = row[29] ? std::string(row[29]) : "";
        orderJson["validatedByCae"] = row[30] ? std::string(row[30]) : "";
        orderJson["sensitiveInfo"] = row[31] ? std::string(row[31]) : "";
        orderJson["encrypted"] = row[32] ? std::string(row[32]) : "";
        orderJson["dongle"] = row[33] ? std::string(row[33]) : "";
        orderJson["dongleId"] = row[34] ? std::string(row[34]) : "";
        orderJson["license"] = row[35] ? std::string(row[35]) : "";
        orderJson["licenseId"] = row[36] ? std::string(row[36]) : "";
        orderJson["featureInit"] = row[37] ? std::string(row[37]) : "";
        orderJson["featureFinal"] = row[38] ? std::string(row[38]) : "";
        
        retVec.push_back(orderJson);
    }
    
    mysql_free_result(res);
    return retVec;
}

std::vector<std::vector<std::pair<std::string,int>>> TicketDAO::selectOrderStatisticsByCondition(int timeRange, std::string ticketType, std::vector<std::string> clientName)
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return {};
    }
    MYSQL* conn = getConnection();
    std::vector<std::vector<std::pair<std::string,int>>> retVec;
    int ret;
    MYSQL_RES* res;
    MYSQL_ROW row;

    //遍历所有的客户，针对每一个客户做一次查询
    for(const std::string& client: clientName)
    {
        std::vector<std::pair<std::string,int>> tmpVec;

        //查询不同时间段的sql分开编写 不同类型的工单分开编写，有的工单没有客户
        std::stringstream ss;

        //根据不同的时间段做不同的筛选 0是筛选出一周的数据 1是筛选出一个月的数据 2是筛选出半年的数据
        if(timeRange == 0)
        {
            ss<<"select DATE(wo.completed_at) as day, count(*) from work_order wo left join delivery_send ds on ds.work_order_id = wo.id left join version_iteration vi on vi.work_order_id = wo.id"
            " left join package_send ps on ps.work_order_id = wo.id where wo.type in('"<<ticketType<<"','直接封装+发送')";

            
            if(ticketType == "版本迭代")
            {
                
            }else if(ticketType == "交付发送")
            {
                ss<<" and (ds.target_customer = '"<<client<<"' or ps.target_customer ='"<<client<<"')";
            }

            ss<<" and wo.completed_at is not null and wo.completed_at >= DATE_SUB(CURDATE(), INTERVAL 7 DAY) GROUP BY day order by day;";

        }else if(timeRange == 1)
        {
            // 只统计本月数据，按自然周分组
            ss << "SELECT "
            << "CONCAT(WEEK(wo.completed_at, 1) - WEEK(DATE_SUB(wo.completed_at, INTERVAL DAYOFMONTH(wo.completed_at)-1 DAY), 1) + 1) AS week_label, "
            << "COUNT(*) "
            << "FROM work_order wo "
            << "LEFT JOIN delivery_send ds ON ds.work_order_id = wo.id "
            << "LEFT JOIN version_iteration vi ON vi.work_order_id = wo.id "
            << "LEFT JOIN package_send ps ON ps.work_order_id = wo.id "
            << "WHERE wo.type IN('" << ticketType << "','直接封装+发送')";

            if(ticketType == "版本迭代")
            {
                // 不加客户筛选
            }
            else if(ticketType == "交付发送")
            {
                ss << " AND (ds.target_customer = '" << client << "' OR ps.target_customer ='" << client << "')";
            }

            ss << " AND wo.completed_at IS NOT NULL "
            << "AND YEAR(wo.completed_at) = YEAR(CURDATE()) "
            << "AND MONTH(wo.completed_at) = MONTH(CURDATE()) "
            << "GROUP BY week_label ORDER BY week_label;";
        }else if(timeRange == 2)
        {
            ss<<"select DATE_FORMAT(completed_at, '%Y-%m') as ym, count(*) from work_order wo left join delivery_send ds on ds.work_order_id = wo.id left join version_iteration vi on vi.work_order_id = wo.id"
            " left join package_send ps on ps.work_order_id = wo.id where wo.type in('"<<ticketType<<"','直接封装+发送')";

            //判断是什么类型的工单
            if(ticketType == "版本迭代")
            {
                
            }else if(ticketType == "交付发送")
            {
                ss<<" and (ds.target_customer = '"<<client<<"' or ps.target_customer ='"<<client<<"')";
            }

            ss<<" and wo.completed_at is not null and wo.completed_at >= DATE_SUB(CURDATE(), INTERVAL 6 MONTH) GROUP BY ym ORDER BY ym;";
        }

        // printf("sql:%s",ss.str().c_str());
        ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
        if (ret) {
            LOG_ERROR("function:selectOrderStatisticsByCondition() 查询 work_order 表失败！失败原因：%s", mysql_error(conn));
            return  {};
        }

        res = mysql_store_result(conn);
        while(row = mysql_fetch_row(res))
        {
            tmpVec.push_back({row[0],atoi(row[1])});
        }
        mysql_free_result(res);
        retVec.push_back(tmpVec);
    }

    return retVec;
}

std::vector<nlohmann::json> TicketDAO::getUserPendingWorkOrders(const std::string& userId)
{
    std::vector<nlohmann::json> result;
    
    if (!ensureConnection()) {
        LOG_ERROR("function:getUserPendingWorkOrders 数据库连接失败");
        return result;
    }
    
    MYSQL* conn = getConnection();
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
    
    // 构建SQL查询语句 - 使用std::string避免缓冲区溢出
    std::ostringstream sqlStream;
    sqlStream << "SELECT DISTINCT "
        << "    wo.id AS work_order_id, "
        << "    wo.type AS work_order_type, "
        << "    wo.status AS work_order_status, "
        << "    wo.created_at, "
        << "    wo.priority, "
        << "    wo.task_priority, "
        << "    wo.approved_at, "
        << "    wo.dispatched_at, "
        << "    wo.model, "
        << "    wo.status_todo, "
        << "    mv.version AS model_version, "
        << "    u_creator.real_name AS creator_name, "
        << "    u_approver.real_name AS approver_name, "
        << "    u_dispatcher.real_name AS dispatcher_name, "
        << "    woe_latest.executor_id, "
        << "    u_executor.real_name AS executor_name, "
        << "    woe_latest.transfer_type AS executor_status, "
        << "    woe_latest.encryption_status, "
        << "    umr.flow_role, "
        << "    ir.coordination_id AS issue_coordination_id, "
        << "    ir.description AS issue_description, "
        << "    ir.reference_file AS issue_reference_file, "
        << "    ira.file_name AS issue_file_name, "
        << "    ir.phenomenon AS issue_phenomenon, "
        << "    ir.remarks AS issue_remarks, "
        << "    vi.coordination_id AS version_coordination_id, "
        << "    vi.update_content AS version_update_content, "
        << "    vi.packaging_requirements AS version_packaging_requirements, "
        << "    vi.interface_changed AS version_interface_changed, "
        << "    vi.new_model_version_id AS version_new_model_version_id, "
        << "    vi.remarks AS version_remarks, "
        << "    vi.matlab_version AS version_matlab_version, "
        << "    ds.target_customer AS delivery_target_customer, "
        << "    ds.validated_by_cae AS delivery_validated_by_cae, "
        << "    ds.sensitive_info AS delivery_sensitive_info, "
        << "    ds.is_encrypted AS delivery_is_encrypted, "
        << "    ds.shell_code AS delivery_shell_code, "
        << "    ds.auth_id AS delivery_authorization_id, "
        << "    ds.remarks AS delivery_remarks, "
        << "    ds.target_delivery_time, "
        << "    u_encryptor.real_name AS delivery_encryptor_name, "
        << "    u_sender.real_name AS delivery_sender_name, "
        << "    woe_encryptor.create_at AS delivery_encryptor_time, "
        << "    woe_sender.finish_at AS delivery_sender_time, "
        << "    ps.coordination_id AS package_coordination_id, "
        << "    ps.update_content AS package_update_content, "
        << "    ps.packaging_requirements AS package_packaging_requirements, "
        << "    ps.interface_changed AS package_interface_changed, "
        << "    ps.target_customer AS package_target_customer, "
        << "    ps.validated_by_cae AS package_validated_by_cae, "
        << "    ps.sensitive_info AS package_sensitive_info, "
        << "    ps.new_model_version_id AS package_new_model_version_id, "
        << "    ps.is_encrypted AS package_is_encrypted, "
        << "    ps.encryption_key AS package_encryption_key, "
        << "    ps.auth_id AS package_product_authorization_id, "
        << "    ps.remarks AS package_remarks, "
        << "    ps.matlab_version AS package_matlab_version, "
        << "    ps.target_delivery_time AS package_target_delivery_time, "
        << "    woe_encryptor.update_version AS package_update_version, "
        << "    fd.description_create AS function_description_create, "
        << "    fd.description_completed AS function_description_completed, "
        << "    fd.model_id AS function_model_id, "
        << "    fd.new_model_version_id AS function_new_model_version_id, "
        << "    fd.matlab_version AS function_matlab_version, "
        << "    owo.description AS other_description, "
        << "    owo.remarks AS other_remarks, "
        << "    wo.to_dispatcher_id, "
        << "    u_to_dispatcher.real_name AS to_dispatcher_name, "
        << "    woe_package.executor_id AS encrypted_executor_id, "
        << "    u_encrypted_executor.real_name AS encrypted_executor_name, "
        << "    wo.encrypted_remark, "
        << "    wo.send_remark, "
        << "    wo.package_remark, "
        << "    woe_package.create_at AS package_create_at, "
        << "    u_package_create.real_name AS package_create_name "
        << "FROM work_order wo "
        << "LEFT JOIN model_version mv ON wo.model_version_id = mv.id "
        << "LEFT JOIN user u_creator ON wo.creator_id = u_creator.username "
        << "LEFT JOIN user u_approver ON wo.approver_id = u_approver.username "
        << "LEFT JOIN user u_dispatcher ON wo.dispatcher_id = u_dispatcher.username "
        << "LEFT JOIN user u_to_dispatcher ON wo.to_dispatcher_id = u_to_dispatcher.username "
        << "LEFT JOIN ( "
        << "    SELECT woe1.work_order_id, woe1.executor_id, woe1.status, woe1.encryption_status, woe1.transfer_type "
        << "    FROM work_order_executor woe1 "
        << "    INNER JOIN ( "
        << "        SELECT work_order_id, MAX(id) as max_id "
        << "        FROM work_order_executor "
        << "        GROUP BY work_order_id "
        << "    ) woe2 ON woe1.work_order_id = woe2.work_order_id AND woe1.id = woe2.max_id "
        << ") woe_latest ON wo.id = woe_latest.work_order_id "
        << "LEFT JOIN user u_executor ON woe_latest.executor_id = u_executor.username "
        << "LEFT JOIN user_multi_role umr ON wo.id = umr.work_order_id "
        << "LEFT JOIN issue_reproduction ir ON wo.id = ir.work_order_id AND wo.type = '问题复现' "
        << "LEFT JOIN issue_reproduction_attachment ira ON wo.id = ira.ticket_id AND wo.type = '问题复现' "
        << "LEFT JOIN version_iteration vi ON wo.id = vi.work_order_id AND wo.type = '版本迭代' "
        << "LEFT JOIN delivery_send ds ON wo.id = ds.work_order_id AND wo.type = '交付发送' "
        << "LEFT JOIN ( "
        << "    SELECT woe_enc.work_order_id, woe_enc.executor_id, woe_enc.create_at,woe_enc.update_version "
        << "    FROM work_order_executor woe_enc "
        << "    INNER JOIN ( "
        << "        SELECT work_order_id, MAX(id) as max_id "
        << "        FROM work_order_executor "
        << "        WHERE status = '加密' OR transfer_type = '加密流转' "
        << "        GROUP BY work_order_id "
        << "    ) woe_enc_max ON woe_enc.work_order_id = woe_enc_max.work_order_id AND woe_enc.id = woe_enc_max.max_id "
        << ") woe_encryptor ON wo.id = woe_encryptor.work_order_id AND wo.type in('交付发送','直接封装+发送') "
        << "LEFT JOIN user u_encryptor ON woe_encryptor.executor_id = u_encryptor.username "
        << "LEFT JOIN ( "
        << "    SELECT woe_send.work_order_id, woe_send.executor_id, woe_send.finish_at "
        << "    FROM work_order_executor woe_send "
        << "    INNER JOIN ( "
        << "        SELECT work_order_id, MAX(id) as max_id "
        << "        FROM work_order_executor "
        << "        WHERE status = '发送'  OR transfer_type = '发送流转' "
        << "        GROUP BY work_order_id "
        << "    ) woe_send_max ON woe_send.work_order_id = woe_send_max.work_order_id AND woe_send.id = woe_send_max.max_id "
        << ") woe_sender ON wo.id = woe_sender.work_order_id AND wo.type in('交付发送','直接封装+发送')"
        << "LEFT JOIN user u_sender ON woe_sender.executor_id = u_sender.username "
        << "LEFT JOIN package_send ps ON wo.id = ps.work_order_id AND wo.type = '直接封装+发送'"
        << "LEFT JOIN ( " 
        << "    SELECT woe_pkg.work_order_id, woe_pkg.update_version, woe_pkg.executor_id , woe_pkg.create_at , woe_pkg.create_id " 
        << "    FROM work_order_executor woe_pkg " 
        << "    INNER JOIN ( " 
        << "        SELECT work_order_id, MAX(id) as max_id " 
        << "        FROM work_order_executor " 
        << "        WHERE status = '封装' OR transfer_type = '封装流转' " 
        << "        GROUP BY work_order_id " 
        << "    ) woe_pkg_max ON woe_pkg.work_order_id = woe_pkg_max.work_order_id AND woe_pkg.id = woe_pkg_max.max_id " 
        << ") woe_package ON wo.id = woe_package.work_order_id AND wo.type = '直接封装+发送'"
        << "LEFT JOIN user u_encrypted_executor ON woe_package.executor_id = u_encrypted_executor.username "
        << "LEFT JOIN user u_package_create ON woe_package.executor_id = u_package_create.username "
        << "LEFT JOIN function_development fd ON wo.id = fd.work_order_id AND wo.type = '功能开发' "
        << "LEFT JOIN other_work_order owo ON wo.id = owo.work_order_id AND wo.type = '其他' "
        << "WHERE " 
        << "    ( " 
        << "        (wo.status = '待审批' AND wo.approver_id = '" << userId << "') " 
        << "        OR " 
        << "        (wo.status = '待分发' AND wo.dispatcher_id = '" << userId << "') " 
        << "        OR " 
        << "        (wo.status = '进行中' AND wo.type != '直接封装+发送' AND woe_latest.executor_id = '" << userId << "') " 
        << "        OR " 
        << "        (wo.status = '进行中' AND wo.type = '直接封装+发送'  AND wo.status_todo= '待封装'  AND woe_package.executor_id = '" << userId << "') " 
        << "        OR " 
        << "        (wo.status = '进行中' AND wo.type = '直接封装+发送'  AND wo.status_todo= '待加密'  AND woe_encryptor.executor_id = '" << userId << "') " 
        << "        OR " 
        << "        (wo.status = '进行中' AND wo.type = '直接封装+发送'  AND wo.status_todo= '待发送'  AND woe_sender.executor_id = '" << userId << "') " 
        << "    ) " 
        << "ORDER BY wo.created_at DESC";
    
    std::string sqlString = sqlStream.str();
    const char* sqlQuery = sqlString.c_str();
    
    LOG_DEBUG("SQL执行: %s", sqlQuery);
    
    ret = mysql_real_query(conn, sqlQuery, (unsigned long)strlen(sqlQuery));
    if (ret) {
        LOG_ERROR("function:getUserPendingWorkOrders SQL查询失败！失败原因：%s", mysql_error(conn));
        return result;
    }
    
    res = mysql_store_result(conn);
    if (!res) {
        LOG_ERROR("function:getUserPendingWorkOrders 获取结果集失败！失败原因：%s", mysql_error(conn));
        return result;
    }
    
    while ((row = mysql_fetch_row(res))) {
        nlohmann::json workOrder;
        
        // 基本工单信息
        workOrder["workOrderId"] = row[0] ? row[0] : "";
        workOrder["workOrderType"] = row[1] ? row[1] : "";
        workOrder["workOrderStatus"] = row[2] ? row[2] : "";
        workOrder["createdAt"] = row[3] ? row[3] : "";
        workOrder["priority"] = row[4] ? row[4] : "";
        workOrder["taskPriority"] = row[5] ? row[5] : "";
        workOrder["approvedAt"] = row[6] ? row[6] : "";
        workOrder["dispatchedAt"] = row[7] ? row[7] : "";
        workOrder["model"] = row[8] ? row[8] : "";
        workOrder["statusTodo"] = row[9] ? row[9] : "";
        workOrder["modelVersion"] = row[10] ? row[10] : "";
        workOrder["creatorName"] = row[11] ? row[11] : "";
        workOrder["approverName"] = row[12] ? row[12] : "";
        workOrder["dispatcherName"] = row[13] ? row[13] : "";
        workOrder["executorId"] = row[14] ? row[14] : "";
        workOrder["executorName"] = row[15] ? row[15] : "";
        workOrder["executorStatus"] = row[16] ? row[16] : "";
        workOrder["encryptionStatus"] = row[17] ? row[17] : "";
        workOrder["flowRole"] = row[18] ? row[18] : "";
        
        // 工单执行人信息
        nlohmann::json executorInfo;
        executorInfo["encryptorName"] = row[40] ? row[40] : "";
        executorInfo["senderName"] = row[41] ? row[41] : "";
        executorInfo["encryptorTime"] = row[42] ? row[42] : "";
        executorInfo["senderTime"] = row[43] ? row[43] : "";
        executorInfo["packageUpdateVersion"] = row[58] ? row[58] : "";
        executorInfo["packageExecutorId"] = row[67] ? row[67] : "";
        executorInfo["packageCreateAt"] = row[73] ? row[73] : "";
        executorInfo["packageCreateName"] = row[74] ? row[74] : "";
        
        // 获取to_dispatcher_name、encrypted_executor_name、encrypted_remark、send_remark和package_remark字段
        unsigned int num_fields = mysql_num_fields(res);
        if (num_fields > 19) {
            workOrder["toDispatcherName"] = row[num_fields - 8] ? row[num_fields - 8] : "";
            executorInfo["encryptedExecutorName"] = row[num_fields - 6] ? row[num_fields - 6] : "";
            workOrder["encryptedRemark"] = row[num_fields - 5] ? row[num_fields - 5] : "";
            workOrder["sendRemark"] = row[num_fields - 4] ? row[num_fields - 4] : "";
            workOrder["packageRemark"] = row[num_fields - 3] ? row[num_fields - 3] : "";
        }
        
        workOrder["executorInfo"] = executorInfo;
        
        // 根据工单类型封装详细信息
        std::string workOrderType = row[1] ? row[1] : "";
        std::string workOrderId = row[0] ? row[0] : "";
        
        if (workOrderType == "问题复现") {
            nlohmann::json issueInfo;
            issueInfo["coordinationId"] = row[19] ? row[19] : "";
            issueInfo["description"] = row[20] ? row[20] : "";
            
            // 处理附件文件路径
            std::string fileName = row[22] ? row[22] : "";
            if (!fileName.empty()) {
                issueInfo["referenceFile"] = "/files/ticket/" + workOrderId + "/" + fileName;
                issueInfo["hasAttachment"] = true;
            } else {
                issueInfo["referenceFile"] = "";
                issueInfo["hasAttachment"] = false;
            }
            issueInfo["fileName"] = row[22] ? row[22] : "";
            issueInfo["phenomenon"] = row[23] ? row[23] : "";
            issueInfo["remarks"] = row[24] ? row[24] : "";
            workOrder["issueReproduction"] = issueInfo;
        }
        else if (workOrderType == "版本迭代") {
            nlohmann::json versionInfo;
            versionInfo["coordinationId"] = row[25] ? row[25] : "";
            versionInfo["updateContent"] = row[26] ? row[26] : "";
            versionInfo["packagingRequirements"] = row[27] ? row[27] : "";
            versionInfo["interfaceChanged"] = row[28] ? (std::string(row[28]) == "1" ? "是" : "否") : "否";
            versionInfo["newModelVersionId"] = row[29] ? row[29] : "";
            versionInfo["remarks"] = row[30] ? row[30] : "";
            versionInfo["matlabVersion"] = row[31] ? row[31] : "";
            workOrder["versionIteration"] = versionInfo;
        }
        else if (workOrderType == "交付发送") {
            nlohmann::json deliveryInfo;
            deliveryInfo["targetCustomer"] = row[32] ? row[32] : "";
            deliveryInfo["validatedByCae"] = row[33] ? (std::string(row[33]) == "1" ? "是" : "否") : "否";
            deliveryInfo["sensitiveInfo"] = row[34] ? row[34] : "";
            deliveryInfo["isEncrypted"] = row[35] ? (std::string(row[35]) == "1" ? "是" : "否") : "否";
            deliveryInfo["shellCode"] = row[36] ? row[36] : "";
            deliveryInfo["authorizationId"] = row[37] ? row[37] : "";
            deliveryInfo["remarks"] = row[38] ? row[38] : "";
            deliveryInfo["targetDeliveryTime"] = row[39] ? row[39] : "";
            workOrder["deliverySend"] = deliveryInfo;
        }
        else if (workOrderType == "直接封装+发送") {
            nlohmann::json packageInfo;
            packageInfo["coordinationId"] = row[44] ? row[44] : "";
            packageInfo["updateContent"] = row[45] ? row[45] : "";
            packageInfo["packagingRequirements"] = row[46] ? row[46] : "";
            packageInfo["interfaceChanged"] = row[47] ? (std::string(row[47]) == "1" ? "是" : "否") : "否";
            packageInfo["targetCustomer"] = row[48] ? row[48] : "";
            packageInfo["validatedByCae"] = row[49] ? (std::string(row[49]) == "1" ? "是" : "否") : "否";
            packageInfo["sensitiveInfo"] = row[50] ? row[50] : "";
            packageInfo["newModelVersionId"] = row[51] ? row[51] : "";
            packageInfo["isEncrypted"] = row[52] ? (std::string(row[52]) == "1" ? "是" : "否") : "否";
            packageInfo["encryptionKey"] = row[53] ? row[53] : "";
            packageInfo["productAuthorizationId"] = row[54] ? row[54] : "";
            packageInfo["remarks"] = row[55] ? row[55] : "";
            packageInfo["matlabVersion"] = row[56] ? row[56] : "";
            packageInfo["targetDeliveryTime"] = row[57] ? row[57] : "";
            workOrder["packageSend"] = packageInfo;
        }
        else if (workOrderType == "功能开发") {
            nlohmann::json functionInfo;
            functionInfo["descriptionCreate"] = row[59] ? row[59] : "";
            functionInfo["descriptionCompleted"] = row[60] ? row[60] : "";
            functionInfo["modelId"] = row[61] ? row[61] : "";
            functionInfo["newModelVersionId"] = row[62] ? row[62] : "";
            functionInfo["matlabVersion"] = row[63] ? row[63] : "";
            workOrder["functionDevelopment"] = functionInfo;
        }
        else if (workOrderType == "其他") {
            nlohmann::json otherInfo;
            otherInfo["description"] = row[64] ? row[64] : "";
            otherInfo["remarks"] = row[65] ? row[65] : "";
            workOrder["otherWorkOrder"] = otherInfo;
        }
        
        // 查询流转信息
        nlohmann::json transferInfo = nlohmann::json::array();
        nlohmann::json versionInfo = nlohmann::json::array();
        nlohmann::json encryptedInfo = nlohmann::json::array();
        nlohmann::json transferInfoDelivery = nlohmann::json::array();
        
        // 根据工单类型查询相应的流转信息
        char transferSql[2048];
        memset(transferSql, 0, sizeof(transferSql));
        
        if (workOrderType == "问题复现" || workOrderType == "功能开发" || workOrderType == "其他") {
            // 只查询完成工单流转
            snprintf(transferSql, sizeof(transferSql),
                "SELECT u1.real_name AS executor_name, u2.real_name AS creator_name, woe.transfer_reason, woe.create_at "
                "FROM work_order_executor woe "
                "LEFT JOIN user u1 ON woe.executor_id = u1.username "
                "LEFT JOIN user u2 ON woe.create_id = u2.username "
                "WHERE woe.work_order_id = %s AND woe.transfer_type = '完成工单流转' "
                "ORDER BY woe.create_at DESC",
                workOrderId.c_str());
        } else if (workOrderType == "版本迭代") {
            // 只查询封装流转
            snprintf(transferSql, sizeof(transferSql),
                "SELECT u1.real_name AS executor_name, u2.real_name AS creator_name, woe.transfer_reason, woe.create_at "
                "FROM work_order_executor woe "
                "LEFT JOIN user u1 ON woe.executor_id = u1.username "
                "LEFT JOIN user u2 ON woe.create_id = u2.username "
                "WHERE woe.work_order_id = %s AND woe.transfer_type = '封装流转' "
                "ORDER BY woe.create_at DESC",
                workOrderId.c_str());
        } else if (workOrderType == "交付发送" || workOrderType == "直接封装+发送") {
             // 查询加密流转、发送流转
             snprintf(transferSql, sizeof(transferSql),
                 "SELECT u1.real_name AS executor_name, u2.real_name AS creator_name, woe.transfer_reason, woe.create_at, woe.transfer_type "
                 "FROM work_order_executor woe "
                 "LEFT JOIN user u1 ON woe.executor_id = u1.username "
                 "LEFT JOIN user u2 ON woe.create_id = u2.username "
                 "WHERE woe.work_order_id = %s AND woe.transfer_type IN ('封装流转','加密流转', '发送流转') "
                 "ORDER BY woe.create_at DESC",
                  workOrderId.c_str());
         }
        
        // 执行流转信息查询
        if (strlen(transferSql) > 0) {
            MYSQL_RES* transferRes;
            MYSQL_ROW transferRow;
            
            int transferRet = mysql_real_query(conn, transferSql, (unsigned long)strlen(transferSql));
            if (!transferRet) {
                transferRes = mysql_store_result(conn);
                if (transferRes) {
                    while ((transferRow = mysql_fetch_row(transferRes))) {
                        nlohmann::json transfer;
                        transfer["transferExecutorID"] = transferRow[0] ? transferRow[0] : "";
                        transfer["transferCreatorID"] = transferRow[1] ? transferRow[1] : "";
                        transfer["transferReason"] = transferRow[2] ? transferRow[2] : "";
                        transfer["transferTime"] = transferRow[3] ? transferRow[3] : "";
                        
                        if (workOrderType == "问题复现" || workOrderType == "功能开发" || workOrderType == "其他") {
                             transferInfo.push_back(transfer);
                         } else if (workOrderType == "版本迭代") {
                             versionInfo.push_back(transfer);
                         } else if (workOrderType == "交付发送" || workOrderType == "直接封装+发送") {
                             // 交付发送工单需要添加特定的交付信息字段
                             std::string transferType = transferRow[4] ? transferRow[4] : "";
                             if (transferType == "封装流转") {
                                 versionInfo.push_back(transfer);
                             } else if (transferType == "加密流转") {
                                 encryptedInfo.push_back(transfer);
                             } else if (transferType == "发送流转") {
                                 transferInfoDelivery.push_back(transfer);
                             }
                         }
                    }
                    mysql_free_result(transferRes);
                }
            }
        }
        
        workOrder["transferInfo"] = transferInfo;// 完成工单流转信息
        workOrder["transferInfoVersion"] = versionInfo;// 封装环节流转信息
        workOrder["transferInfoEncrypted"] = encryptedInfo;// 加密环节流转信息
        workOrder["transferInfoDelivery"] = transferInfoDelivery;// 发送环节流转信息
        
        result.push_back(workOrder);    }
    
    mysql_free_result(res);
    return result;
}

std::vector<nlohmann::json> TicketDAO::selectOrderByConditionWithDetails(const std::map<std::string, std::string>& filter, int offset, int count)
{
    std::vector<nlohmann::json> result;
    
    if (!ensureConnection()) {
        LOG_ERROR("function:selectOrderByConditionWithDetails 数据库连接失败");
        return result;
    }
    
    MYSQL* conn = getConnection();
    
    // 构建基础SQL查询语句（基于getUserPendingWorkOrders的结构）
    std::stringstream ss;
    ss << "SELECT DISTINCT "
       << "    wo.id AS work_order_id, "
       << "    wo.type AS work_order_type, "
       << "    wo.status AS work_order_status, "
       << "    wo.created_at, "
       << "    wo.priority, "
       << "    wo.task_priority, "
       << "    wo.approved_at, "
       << "    wo.dispatched_at, "
       << "    wo.model, "
       << "    wo.status_todo, "
       << "    mv.version AS model_version, "
       << "    u_creator.real_name AS creator_name, "
       << "    u_approver.real_name AS approver_name, "
       << "    u_dispatcher.real_name AS dispatcher_name, "
       << "    wo.to_dispatcher_id, "
       << "    u_to_dispatcher.real_name AS to_dispatcher_name, "
       << "    wo.encrypted_remark, "
       << "    wo.send_remark, "
       << "    wo.package_remark, "
       << "    woe_latest.executor_id, "
       << "    u_executor.real_name AS executor_name, "
       << "    woe_latest.transfer_type AS executor_status, "
       << "    woe_latest.encryption_status, "
       << "    umr.flow_role, "
       << "    woe_encrypted.executor_id AS encrypted_executor_id, "
       << "    u_encrypted.real_name AS encrypted_executor_name, "
       << "    woe_sender.executor_id AS sender_executor_id, "
       << "    u_sender.real_name AS sender_executor_name, "
       << "    woe_encrypted.create_at AS encrypted_create_at, "
       << "    woe_sender.create_at AS sender_create_at, "
       << "    woe_package.update_version AS package_update_version, "
       << "    woe_package.executor_id AS package_executor_id, "
       << "    woe_package.create_at AS package_create_at, "
       << "    u_package.real_name AS package_create_name, "
       << "    woe_encrypted_dynamic.executor_id AS encrypted_executor_name_dynamic, "
       << "    ir.coordination_id AS issue_coordination_id, "
       << "    ir.description AS issue_description, "
       << "    ir.reference_file AS issue_reference_file, "
       << "    ira.file_name AS issue_file_name, "
       << "    ir.phenomenon AS issue_phenomenon, "
       << "    ir.remarks AS issue_remarks, "
       << "    vi.coordination_id AS version_coordination_id, "
       << "    vi.update_content AS version_update_content, "
       << "    vi.packaging_requirements AS version_packaging_requirements, "
       << "    vi.interface_changed AS version_interface_changed, "
       << "    vi.new_model_version_id AS version_new_model_version_id, "
       << "    vi.remarks AS version_remarks, "
       << "    vi.matlab_version AS version_matlab_version, "
       << "    ds.target_customer AS delivery_target_customer, "
       << "    ds.validated_by_cae AS delivery_validated_by_cae, "
       << "    ds.sensitive_info AS delivery_sensitive_info, "
       << "    ds.is_encrypted AS delivery_is_encrypted, "
       << "    ds.shell_code AS delivery_shell_code, "
       << "    ds.auth_id AS delivery_authorization_id, "
       << "    ds.remarks AS delivery_remarks, "
       << "    ds.target_delivery_time, "
       << "    ps.coordination_id AS package_coordination_id, "
       << "    ps.update_content AS package_update_content, "
       << "    ps.packaging_requirements AS package_packaging_requirements, "
       << "    ps.interface_changed AS package_interface_changed, "
       << "    ps.target_customer AS package_target_customer, "
       << "    ps.validated_by_cae AS package_validated_by_cae, "
       << "    ps.sensitive_info AS package_sensitive_info, "
       << "    ps.new_model_version_id AS package_new_model_version_id, "
       << "    ps.is_encrypted AS package_is_encrypted, "
       << "    ps.encryption_key AS package_encryption_key, "
       << "    ps.auth_id AS package_product_authorization_id, "
       << "    ps.remarks AS package_remarks, "
       << "    ps.matlab_version AS package_matlab_version, "
       << "    ps.target_delivery_time AS package_target_delivery_time, "
       << "    fd.description_create AS function_description_create, "
       << "    fd.description_completed AS function_description_completed, "
       << "    fd.model_id AS function_model_id, "
       << "    fd.new_model_version_id AS function_new_model_version_id, "
       << "    fd.matlab_version AS function_matlab_version, "
       << "    owo.description AS other_description, "
       << "    owo.remarks AS other_remarks, "
       << "    woe_encryptor.executor_id AS delivery_encryptor_name, "
       << "    woe_sender.executor_id AS delivery_sender_name, "
       << "    woe_encryptor.create_at AS delivery_encryptor_time, "
       << "    woe_sender.create_at AS delivery_sender_time, "
       << "    wo.completed_at, "
       << "    mv2.version AS new_model_version ,"
       << "    mv3.version AS new_model_version_vi, "
       << "    mv4.version AS new_model_version_ps "
       << "FROM work_order wo "
       << "LEFT JOIN model_version mv ON wo.model_version_id = mv.id "
       << "LEFT JOIN user u_creator ON wo.creator_id = u_creator.username "
       << "LEFT JOIN user u_approver ON wo.approver_id = u_approver.username "
       << "LEFT JOIN user u_dispatcher ON wo.dispatcher_id = u_dispatcher.username "
       << "LEFT JOIN user u_to_dispatcher ON wo.to_dispatcher_id = u_to_dispatcher.username "
       << "LEFT JOIN ( "
       << "    SELECT woe1.work_order_id, woe1.executor_id, woe1.status, woe1.encryption_status, woe1.transfer_type "
       << "    FROM work_order_executor woe1 "
       << "    INNER JOIN ( "
       << "        SELECT work_order_id, MAX(id) as max_id "
       << "        FROM work_order_executor "
       << "        GROUP BY work_order_id "
       << "    ) woe2 ON woe1.work_order_id = woe2.work_order_id AND woe1.id = woe2.max_id "
       << ") woe_latest ON wo.id = woe_latest.work_order_id "
       << "LEFT JOIN user u_executor ON woe_latest.executor_id = u_executor.username "
       << "LEFT JOIN user_multi_role umr ON wo.id = umr.work_order_id "
       << "LEFT JOIN ( "
       << "    SELECT woe_enc.work_order_id, woe_enc.executor_id, woe_enc.create_at "
       << "    FROM work_order_executor woe_enc "
       << "    INNER JOIN ( "
       << "        SELECT work_order_id, MAX(id) as max_id "
       << "        FROM work_order_executor "
       << "        WHERE status = '加密' OR transfer_type = '加密流转'"
       << "        GROUP BY work_order_id "
       << "    ) woe_enc_max ON woe_enc.work_order_id = woe_enc_max.work_order_id AND woe_enc.id = woe_enc_max.max_id "
       << ") woe_encrypted ON wo.id = woe_encrypted.work_order_id "
       << "LEFT JOIN user u_encrypted ON woe_encrypted.executor_id = u_encrypted.username "
       << "LEFT JOIN ( "
       << "    SELECT woe_send.work_order_id, woe_send.executor_id, woe_send.create_at "
       << "    FROM work_order_executor woe_send "
       << "    INNER JOIN ( "
       << "        SELECT work_order_id, MAX(id) as max_id "
       << "        FROM work_order_executor "
       << "        WHERE status = '发送' OR transfer_type = '发送流转'"
       << "        GROUP BY work_order_id "
       << "    ) woe_send_max ON woe_send.work_order_id = woe_send_max.work_order_id AND woe_send.id = woe_send_max.max_id "
       << ") woe_sender ON wo.id = woe_sender.work_order_id "
       << "LEFT JOIN user u_sender ON woe_sender.executor_id = u_sender.username "
       << "LEFT JOIN ( "
       << "    SELECT woe_pkg.work_order_id, woe_pkg.executor_id, woe_pkg.create_at, woe_pkg.update_version "
       << "    FROM work_order_executor woe_pkg "
       << "    INNER JOIN ( "
       << "        SELECT work_order_id, MAX(id) as max_id "
       << "        FROM work_order_executor "
       << "        WHERE status = '封装' OR transfer_type = '封装流转'"
       << "        GROUP BY work_order_id "
       << "    ) woe_pkg_max ON woe_pkg.work_order_id = woe_pkg_max.work_order_id AND woe_pkg.id = woe_pkg_max.max_id "
       << ") woe_package ON wo.id = woe_package.work_order_id AND wo.type = '直接封装+发送' "
       << "LEFT JOIN user u_package ON woe_package.executor_id = u_package.username "
       << "LEFT JOIN ( "
       << "    SELECT woe_enc_dyn.work_order_id, woe_enc_dyn.executor_id "
       << "    FROM work_order_executor woe_enc_dyn "
       << "    INNER JOIN ( "
       << "        SELECT work_order_id, MAX(id) as max_id "
       << "        FROM work_order_executor "
       << "        WHERE status = '加密' OR transfer_type = '加密流转'"
       << "        GROUP BY work_order_id "
       << "    ) woe_enc_dyn_max ON woe_enc_dyn.work_order_id = woe_enc_dyn_max.work_order_id AND woe_enc_dyn.id = woe_enc_dyn_max.max_id "
       << ") woe_encrypted_dynamic ON wo.id = woe_encrypted_dynamic.work_order_id "
       << "LEFT JOIN issue_reproduction ir ON wo.id = ir.work_order_id AND wo.type = '问题复现' "
       << "LEFT JOIN issue_reproduction_attachment ira ON wo.id = ira.ticket_id AND wo.type = '问题复现' "
       << "LEFT JOIN version_iteration vi ON wo.id = vi.work_order_id AND wo.type = '版本迭代' "
       << "LEFT JOIN model_version mv3 ON vi.new_model_version_id = mv3.id "
       << "LEFT JOIN delivery_send ds ON wo.id = ds.work_order_id AND wo.type = '交付发送' "
       << "LEFT JOIN package_send ps ON wo.id = ps.work_order_id AND wo.type = '直接封装+发送' "
       << "LEFT JOIN model_version mv4 ON ps.new_model_version_id = mv4.id "
       << "LEFT JOIN function_development fd ON wo.id = fd.work_order_id AND wo.type = '功能开发' "
       << "LEFT JOIN model_version mv2 ON fd.new_model_version_id = mv2.id AND wo.type = '功能开发' "
       << "LEFT JOIN other_work_order owo ON wo.id = owo.work_order_id AND wo.type = '其他' "
       << "LEFT JOIN ( "
       << "    SELECT woe_enc.work_order_id, woe_enc.executor_id, woe_enc.create_at "
       << "    FROM work_order_executor woe_enc "
       << "    INNER JOIN ( "
       << "        SELECT work_order_id, MAX(id) as max_id "
       << "        FROM work_order_executor "
       << "        WHERE status = '加密' "
       << "        GROUP BY work_order_id "
       << "    ) woe_enc_max ON woe_enc.work_order_id = woe_enc_max.work_order_id AND woe_enc.id = woe_enc_max.max_id "
       << "    WHERE woe_enc.status = '加密' "
        << ") woe_encryptor ON wo.id = woe_encryptor.work_order_id ";
    
    // 添加过滤条件（参考getUserPendingWorkOrders的逻辑）
    std::set<std::string> intSet={"id","creator_id","model_version_id","approver_id","dispatcher_id","executor_id"};
    bool first = true;
    
    if(!filter.empty()) {
        ss << " WHERE ";
    }
    
    // 处理filterMineFlag标志，根据userId过滤与当前用户相关的工单
    auto filterMineFlagIt = filter.find("filterMineFlag");
    auto userIdIt = filter.find("userId");
    if (filterMineFlagIt != filter.end() && filterMineFlagIt->second == "true" && userIdIt != filter.end()) {
        if (!first) ss << " AND ";
        ss << "(";
        ss << "wo.creator_id = '" << userIdIt->second << "' ";
        ss << "OR wo.approver_id = '" << userIdIt->second << "' ";
        ss << "OR wo.dispatcher_id = '" << userIdIt->second << "' ";
        ss << "OR wo.to_dispatcher_id = '" << userIdIt->second << "' ";
        ss << "OR woe_latest.executor_id = '" << userIdIt->second << "' ";
        ss << "OR woe_sender.executor_id = '" << userIdIt->second << "' ";
        ss << "OR woe_encrypted.executor_id = '" << userIdIt->second << "' ";
        ss << "OR woe_package.executor_id = '" << userIdIt->second << "'";
        ss << ")";
        first = false;
    }
    
    for(auto ele: filter) {
        // 跳过filterMineFlag和userId，它们已经在上面处理过了
        if(ele.first == "filterMineFlag" || ele.first == "userId") {
            continue;
        }
        
        if(!first) ss << " AND ";
        
        if(intSet.find(ele.first) != intSet.end()) {
            if(ele.first == "executor_id") {
                // 参考getUserPendingWorkOrders的执行人查询逻辑
                ss << "(";
                ss << "(wo.status != '进行中' AND woe_latest." << ele.first << " = '" << ele.second << "') ";
                ss << "OR ";
                ss << "(wo.status = '进行中' AND wo.type != '直接封装+发送' AND woe_latest." << ele.first << " = '" << ele.second << "') ";
                ss << "OR ";
                ss << "(wo.status = '进行中' AND wo.type = '直接封装+发送' AND woe_package.executor_id = '" << ele.second << "') ";
                ss << "OR ";
                ss << "(wo.status = '进行中' AND wo.type IN ('交付发送', '直接封装+发送') AND woe_encrypted_dynamic.executor_id = '" << ele.second << "')";
                ss << ")";
            } else if(ele.first == "id") {
                ss << "wo." << ele.first << " = " << ele.second;
            } else {
                ss << "wo." << ele.first << " = '" << ele.second << "'";
            }
        } else {
            if(ele.first == "model") {
                // 处理以逗号分割的模型名字符串，使用IN查询
                ss << "wo." << ele.first << " IN (";
                std::string models = ele.second;
                std::stringstream modelStream(models);
                std::string model;
                bool firstModel = true;
                while(std::getline(modelStream, model, ',')) {
                    if(!firstModel) ss << ",";
                    ss << "'" << model << "'";
                    firstModel = false;
                }
                ss << ")";
            } else if(ele.first == "startDate") {
                // 添加目标交付时间的开始日期过滤
                ss << "((ds.target_delivery_time IS NOT NULL AND ds.target_delivery_time >= '" << ele.second << "') ";
                ss << "OR (ps.target_delivery_time IS NOT NULL AND ps.target_delivery_time >= '" << ele.second << "'))";
            } else if(ele.first == "endDate") {
                // 添加目标交付时间的结束日期过滤
                ss << "((ds.target_delivery_time IS NOT NULL AND ds.target_delivery_time <= '" << ele.second << "') ";
                ss << "OR (ps.target_delivery_time IS NOT NULL AND ps.target_delivery_time <= '" << ele.second << "'))";
            } else {
                ss << "wo." << ele.first << " LIKE '%" << ele.second << "%'";
            }
        }
        first = false;
    }
    
    // 添加排序和分页
    ss << " ORDER BY wo.created_at DESC";
    ss << " LIMIT " << count << " OFFSET " << offset;
    
    LOG_DEBUG("SQL执行: %s", ss.str().c_str());
    
    int ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    if (ret) {
        LOG_ERROR("function:selectOrderByConditionWithDetails SQL查询失败！失败原因：%s", mysql_error(conn));
        return result;
    }
    
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        LOG_ERROR("function:selectOrderByConditionWithDetails 获取结果集失败！失败原因：%s", mysql_error(conn));
        return result;
    }
    
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        nlohmann::json workOrder;
        
        // 基本工单信息（与getUserPendingWorkOrders相同的字段映射）
        workOrder["workOrderId"] = row[0] ? row[0] : "";
        workOrder["workOrderType"] = row[1] ? row[1] : "";
        workOrder["workOrderStatus"] = row[2] ? row[2] : "";
        workOrder["createdAt"] = row[3] ? row[3] : "";
        workOrder["priority"] = row[4] ? row[4] : "";
        workOrder["taskPriority"] = row[5] ? row[5] : "";
        workOrder["approvedAt"] = row[6] ? row[6] : "";
        workOrder["dispatchedAt"] = row[7] ? row[7] : "";
        workOrder["model"] = row[8] ? row[8] : "";
        workOrder["statusTodo"] = row[9] ? row[9] : "";
        workOrder["modelVersion"] = row[10] ? row[10] : "";
        workOrder["creatorName"] = row[11] ? row[11] : "";
        workOrder["approverName"] = row[12] ? row[12] : "";
        workOrder["dispatcherName"] = row[13] ? row[13] : "";
        workOrder["toDispatcherId"] = row[14] ? row[14] : "";
        workOrder["toDispatcherName"] = row[15] ? row[15] : "";
        workOrder["encryptedRemark"] = row[16] ? row[16] : "";
        workOrder["sendRemark"] = row[17] ? row[17] : "";
        workOrder["packageRemark"] = row[18] ? row[18] : "";
        workOrder["executorId"] = row[19] ? row[19] : "";
        workOrder["executorName"] = row[20] ? row[20] : "";
        workOrder["executorStatus"] = row[21] ? row[21] : "";
        workOrder["encryptionStatus"] = row[22] ? row[22] : "";
        workOrder["flowRole"] = row[23] ? row[23] : "";
        workOrder["completedAt"] = row[81] ? row[81] : "";
        
        // 统一的执行人信息（参考getUserPendingWorkOrders的executorInfo结构）
        nlohmann::json executorInfo;
        executorInfo["encryptedExecutorName"] = row[25] ? row[25] : "";
        executorInfo["senderExecutorName"] = row[27] ? row[27] : "";
        executorInfo["encryptedCreateAt"] = row[28] ? row[28] : "";
        executorInfo["senderCreateAt"] = row[29] ? row[29] : "";
        executorInfo["packageUpdateVersion"] = row[30] ? row[30] : "";
        executorInfo["packageExecutorId"] = row[31] ? row[31] : "";
        executorInfo["packageCreateAt"] = row[32] ? row[32] : "";
        executorInfo["packageCreateName"] = row[33] ? row[33] : "";
        executorInfo["encryptedExecutorNameDynamic"] = row[34] ? row[34] : "";
        workOrder["executorInfo"] = executorInfo;
        
        // 根据工单类型封装详细信息（与getUserPendingWorkOrders相同的逻辑）
        std::string workOrderType = row[1] ? row[1] : "";
        std::string workOrderId = row[0] ? row[0] : "";
        
        if (workOrderType == "问题复现") {
            nlohmann::json issueInfo;
            issueInfo["coordinationId"] = row[35] ? row[35] : "";
            issueInfo["description"] = row[36] ? row[36] : "";
            
            // 处理附件文件路径
            std::string fileName = row[38] ? row[38] : "";
            if (!fileName.empty()) {
                issueInfo["referenceFile"] = "/files/ticket/" + workOrderId + "/" + fileName;
                issueInfo["hasAttachment"] = true;
            } else {
                issueInfo["referenceFile"] = "";
                issueInfo["hasAttachment"] = false;
            }
            issueInfo["fileName"] = row[38] ? row[38] : "";
            issueInfo["phenomenon"] = row[39] ? row[39] : "";
            issueInfo["remarks"] = row[40] ? row[40] : "";
            workOrder["issueReproduction"] = issueInfo;
        }
        else if (workOrderType == "版本迭代") {
            nlohmann::json versionInfo;
            versionInfo["coordinationId"] = row[41] ? row[41] : "";
            versionInfo["updateContent"] = row[42] ? row[42] : "";
            versionInfo["packagingRequirements"] = row[43] ? row[43] : "";
            versionInfo["interfaceChanged"] = row[44] ? (std::string(row[44]) == "1" ? true : false) : false;
            versionInfo["newModelVersionId"] = row[45] ? row[45] : "";
            versionInfo["remarks"] = row[46] ? row[46] : "";
            versionInfo["matlabVersion"] = row[47] ? row[47] : "";
            versionInfo["newModelVersion"] = row[83] ? row[83] : "";
            workOrder["versionIteration"] = versionInfo;
        }
        else if (workOrderType == "交付发送") {
            nlohmann::json deliveryInfo;
            deliveryInfo["targetCustomer"] = row[48] ? row[48] : "";
            deliveryInfo["validatedByCae"] = row[49] ? (std::string(row[49]) == "1" ? "是" : "否") : "否";
            deliveryInfo["sensitiveInfo"] = row[50] ? row[50] : "";
            deliveryInfo["isEncrypted"] = row[51] ? (std::string(row[51]) == "1" ? "是" : "否") : "否";
            deliveryInfo["shellCode"] = row[52] ? row[52] : "";
            deliveryInfo["authorizationId"] = row[53] ? row[53] : "";
            deliveryInfo["remarks"] = row[54] ? row[54] : "";
            deliveryInfo["targetDeliveryTime"] = row[55] ? row[55] : "";
            deliveryInfo["encryptorName"] = row[25] ? row[25] : "";
            deliveryInfo["senderName"] = row[27] ? row[27] : "";
            deliveryInfo["encryptorTime"] = row[28] ? row[28] : "";
            deliveryInfo["senderTime"] = row[29] ? row[29] : "";
            workOrder["deliverySend"] = deliveryInfo;
        }
        else if (workOrderType == "直接封装+发送") {
            nlohmann::json packageInfo;
            packageInfo["coordinationId"] = row[56] ? row[56] : "";
            packageInfo["updateContent"] = row[57] ? row[57] : "";
            packageInfo["packagingRequirements"] = row[58] ? row[58] : "";
            packageInfo["interfaceChanged"] = row[59] ? (std::string(row[59]) == "1" ? true : false) : false;
            packageInfo["targetCustomer"] = row[60] ? row[60] : "";
            packageInfo["validatedByCae"] = row[61] ? (std::string(row[61]) == "1" ? "是" : "否") : "否";
            packageInfo["sensitiveInfo"] = row[62] ? row[62] : "";
            packageInfo["newModelVersionId"] = row[63] ? row[63] : "";
            packageInfo["isEncrypted"] = row[64] ? (std::string(row[64]) == "1" ? "是" : "否") : "否";
            packageInfo["encryptionKey"] = row[65] ? row[65] : "";
            packageInfo["productAuthorizationId"] = row[66] ? row[66] : "";
            packageInfo["remarks"] = row[67] ? row[67] : "";
            packageInfo["matlabVersion"] = row[68] ? row[68] : "";
            packageInfo["targetDeliveryTime"] = row[69] ? row[69] : "";
            packageInfo["newModelVersion"] = row[84] ? row[84] : "";
            workOrder["packageSend"] = packageInfo;
        }
        else if (workOrderType == "功能开发") {
            nlohmann::json functionInfo;
            functionInfo["descriptionCreate"] = row[70] ? row[70] : "";
            functionInfo["descriptionCompleted"] = row[71] ? row[71] : "";
            functionInfo["modelId"] = row[72] ? row[72] : "";
            functionInfo["newModelVersionId"] = row[73] ? row[73] : "";
            functionInfo["matlabVersion"] = row[74] ? row[74] : "";
            functionInfo["newModelVersion"] = row[82] ? row[82] : "";

            workOrder["functionDevelopment"] = functionInfo;
        }
        else if (workOrderType == "其他") {
            nlohmann::json otherInfo;
            otherInfo["description"] = row[75] ? row[75] : "";
            otherInfo["remarks"] = row[76] ? row[76] : "";
            workOrder["otherWorkOrder"] = otherInfo;
        }
        
        // 查询流转信息（与getUserPendingWorkOrders相同的逻辑）
        nlohmann::json transferInfo = nlohmann::json::array();
        nlohmann::json versionInfo = nlohmann::json::array();
        nlohmann::json encryptedInfo = nlohmann::json::array();
        nlohmann::json transferInfoDelivery = nlohmann::json::array();
        
        // 根据工单类型查询相应的流转信息
        char transferSql[2048];
        memset(transferSql, 0, sizeof(transferSql));
        
        if (workOrderType == "问题复现" || workOrderType == "功能开发" || workOrderType == "其他") {
            // 只查询完成工单流转
            snprintf(transferSql, sizeof(transferSql),
                "SELECT u1.real_name AS executor_name, u2.real_name AS creator_name, woe.transfer_reason, woe.create_at "
                "FROM work_order_executor woe "
                "LEFT JOIN user u1 ON woe.executor_id = u1.username "
                "LEFT JOIN user u2 ON woe.create_id = u2.username "
                "WHERE woe.work_order_id = %s AND woe.transfer_type = '完成工单流转' "
                "ORDER BY woe.create_at DESC",
                workOrderId.c_str());
        } else if (workOrderType == "版本迭代") {
            // 只查询封装流转
            snprintf(transferSql, sizeof(transferSql),
                "SELECT u1.real_name AS executor_name, u2.real_name AS creator_name, woe.transfer_reason, woe.create_at "
                "FROM work_order_executor woe "
                "LEFT JOIN user u1 ON woe.executor_id = u1.username "
                "LEFT JOIN user u2 ON woe.create_id = u2.username "
                "WHERE woe.work_order_id = %s AND woe.transfer_type = '封装流转' "
                "ORDER BY woe.create_at DESC",
                workOrderId.c_str());
        } else if (workOrderType == "交付发送" || workOrderType == "直接封装+发送") {
             // 查询加密流转、发送流转
             snprintf(transferSql, sizeof(transferSql),
                 "SELECT u1.real_name AS executor_name, u2.real_name AS creator_name, woe.transfer_reason, woe.create_at, woe.transfer_type "
                 "FROM work_order_executor woe "
                 "LEFT JOIN user u1 ON woe.executor_id = u1.username "
                 "LEFT JOIN user u2 ON woe.create_id = u2.username "
                 "WHERE woe.work_order_id = %s AND woe.transfer_type IN ('封装流转','加密流转', '发送流转') "
                 "ORDER BY woe.create_at DESC",
                  workOrderId.c_str());
         }
        
        // 执行流转信息查询
        if (strlen(transferSql) > 0) {
            MYSQL_RES* transferRes;
            MYSQL_ROW transferRow;
            
            int transferRet = mysql_real_query(conn, transferSql, (unsigned long)strlen(transferSql));
            if (!transferRet) {
                transferRes = mysql_store_result(conn);
                if (transferRes) {
                    while ((transferRow = mysql_fetch_row(transferRes))) {
                        nlohmann::json transfer;
                        transfer["transferExecutorName"] = transferRow[0] ? transferRow[0] : "";
                        transfer["transferCreatorName"] = transferRow[1] ? transferRow[1] : "";
                        transfer["transferReason"] = transferRow[2] ? transferRow[2] : "";
                        transfer["transferTime"] = transferRow[3] ? transferRow[3] : "";
                        
                        if (workOrderType == "问题复现" || workOrderType == "功能开发" || workOrderType == "其他") {
                             transferInfo.push_back(transfer);
                         } else if (workOrderType == "版本迭代") {
                             versionInfo.push_back(transfer);
                         } else if (workOrderType == "交付发送" || workOrderType == "直接封装+发送") {
                             // 交付发送工单需要添加特定的交付信息字段
                             std::string transferType = transferRow[4] ? transferRow[4] : "";
                             if (transferType == "封装流转") {
                                 versionInfo.push_back(transfer);
                             } else if (transferType == "加密流转") {
                                 encryptedInfo.push_back(transfer);
                             } else if (transferType == "发送流转") {
                                 transferInfoDelivery.push_back(transfer);
                             }
                         }
                    }
                    mysql_free_result(transferRes);
                }
            }
        }
        
        workOrder["transferInfo"] = transferInfo;// 完成工单流转信息
        workOrder["transferInfoVersion"] = versionInfo;// 封装环节流转信息
        workOrder["transferInfoEncrypted"] = encryptedInfo;// 加密环节流转信息
        workOrder["transferInfoDelivery"] = transferInfoDelivery;// 发送环节流转信息
        
        result.push_back(workOrder);
    }
    
    mysql_free_result(res);
    return result;
}

bool TicketDAO::completeSendTicket(const Ticket& ticket)
{
    MYSQL* conn = getConnection();
    if (!conn) {
        LOG_ERROR("function:completeSendTicket 获取数据库连接失败");
        return false;
    }
    
    // 开始事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:completeSendTicket 开始事务失败！失败原因：%s", mysql_error(conn));
        return false;
    }
    
    char sql[SQL_MAX];
    int ret;
    // 获取当前时间 (UTF-8时区)
    std::time_t now = std::time(nullptr);
    char timeStr[20];
    // 使用UTC时间，确保UTF-8编码兼容性
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::gmtime(&now));
    std::string completedTime = timeStr;
    try {
        // 1. 更新工单状态为已完成
        std::string remarkStr = "";
        if (ticket.ticketType == "交付发送") {
            const TicketDelivery& deliveryTicket = dynamic_cast<const TicketDelivery&>(ticket);
            remarkStr = deliveryTicket.remark;
        snprintf(sql, SQL_MAX, 
            "UPDATE work_order SET status = '已完成', completed_at = '%s', send_remark = '%s' WHERE id = %d",
            ticket.completedTime.c_str(), remarkStr.c_str(), ticket.id);
        } else if (ticket.ticketType == "版本迭代+交付发送") {
            const TicketPackage& packageTicket = dynamic_cast<const TicketPackage&>(ticket);
            remarkStr = packageTicket.remark;
        snprintf(sql, SQL_MAX, 
            "UPDATE work_order SET status = '已完成', completed_at = '%s', send_remark = '%s' WHERE id = %d",
            ticket.completedTime.c_str(), remarkStr.c_str(), ticket.id);
        }else{
        
        snprintf(sql, SQL_MAX, 
            "UPDATE work_order SET status = '已完成', completed_at = '%s', package_remark = '%s' WHERE id = %d",
            ticket.completedTime.c_str(),ticket.packageRemark.c_str(), ticket.id);
        }
    
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completeSendTicket 更新工单状态失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        // 2. 根据工单id插入完成记录
        snprintf(sql, SQL_MAX,
            "INSERT INTO work_order_executor (work_order_id, executor_id, status, finish_at) VALUES (%d, %s, '发送', '%s')",
            ticket.id, ticket.executorId.c_str(), completedTime.c_str());
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completeSendTicket 插入工单执行记录失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        // 更新相应的详细信息表
        // const TicketDelivery& deliveryTicket = dynamic_cast<const TicketDelivery&>(ticket);
        // snprintf(sql, SQL_MAX, "UPDATE work_order SET completed_at = '%s', status = '已完成',status_todo = null WHERE id = %d",
        //        completedTime.c_str(), deliveryTicket.id);
        
        // 3. 根据工单类型更新相应的详细信息表
        // if (ticket.ticketType == "交付发送") {

        // } else if (ticket.ticketType == "版本迭代") {
        //     // TODO 备注对齐

        // } else if (ticket.ticketType == "直接封装+发送") {
        //     const TicketPackage& packageTicket = dynamic_cast<const TicketPackage&>(ticket);
        //     snprintf(sql, SQL_MAX,
        //         "UPDATE package_send SET send_executor_id = %s, send_time = '%s', send_remark = '%s' WHERE work_order_id = %d",
        //         packageTicket.sendExecutorId.c_str(), packageTicket.completedTime.c_str(),
        //         packageTicket.remark.c_str(), packageTicket.id);
        // } else {
        //     LOG_ERROR("function:completeSendTicket 不支持的工单类型: %s", ticket.ticketType.c_str());
        //     mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        //     return false;
        // }
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completeSendTicket 更新详细信息表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        // 如果是版本迭代工单，需要更新模型版本
        if (ticket.ticketType == "版本迭代") {
            auto& t = dynamic_cast<const TicketVersion&>(ticket);
            snprintf(sql, SQL_MAX, "INSERT INTO model_version(version,model) VALUES('%s', '%s');", t.newModelVersion.c_str(),t.model.c_str());	
            ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
            if (ret) {
                LOG_ERROR("function:addModelVersion 添加模型版本失败！失败原因：%s", mysql_error(conn));
                return false;
            }
                // 获取插入的自增ID
            unsigned long long insertedId = mysql_insert_id(conn);
            LOG_INFO("新插入的model_version记录ID: %llu", insertedId);
            //
            
        //根据模型以及模型版本去模型版本表里把具体的模型版本ID查出来
            snprintf(sql, SQL_MAX, "update version_iteration set new_model_version_id = %llu,remarks = '%s' where work_order_id = %d;",insertedId,t.remark.c_str(),t.Ticket::id);
            ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
            if (ret) {  
                LOG_ERROR("function:completeConcreteTicket 修改version_iteration表失败！失败原因：%s", mysql_error(conn));
                mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
                return false;
            }
        
    
        }
        
        // 提交事务
        if (mysql_real_query(conn, "COMMIT", strlen("COMMIT"))) {
            LOG_ERROR("function:completeSendTicket 提交事务失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        LOG_INFO("function:completeSendTicket 工单发送完成成功，工单ID: %d", ticket.id);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("function:completeSendTicket 处理过程中发生异常: %s", e.what());
        mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
}

bool TicketDAO::completePackageSendTicket(const TicketPackage& ticket)
{
    MYSQL* conn = getConnection();
    if (!conn) {
        LOG_ERROR("function:completePackageSendTicket 获取数据库连接失败");
        return false;
    }
    
    // 开始事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:completePackageSendTicket 开始事务失败！失败原因：%s", mysql_error(conn));
        return false;
    }
    
    char sql[SQL_MAX];
    int ret;
    // 获取当前时间
    std::time_t now = std::time(nullptr);
    char timeStr[20];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::gmtime(&now));
    std::string completedTime = timeStr;
    
    try {
        // 1. 更新工单状态为进行中（封装步骤完成后进入加密阶段）        
        snprintf(sql, SQL_MAX, 
            "UPDATE work_order SET  status_todo = '待加密', package_remark = '%s' WHERE id = %d",
            ticket.remark.c_str(), ticket.id);
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completePackageSendTicket 更新工单状态失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        // 2.1 插入工单执行记录的完成时间（封装步骤）
        snprintf(sql, SQL_MAX,
            "INSERT INTO work_order_executor (work_order_id, executor_id, create_id, update_version, status, create_at) VALUES (%d, '%s', '%s', '%s', '加密', NOW())", 
            ticket.id, ticket.encryptedExecutorId.c_str(), ticket.executorId.c_str(), ticket.newModelVersion.c_str());
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completePackageSendTicket 更新工单执行记录失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        // 2.2 插入工单执行记录的完成时间（流转步骤）
        // snprintf(sql, SQL_MAX,
        //     "INSERT INTO work_order_executor (work_order_id, executor_id, create_id, update_version, status, transfer_type, create_at) VALUES (%d, '%s', '%s', '%s', '流转', '加密流转', NOW())", 
        //     ticket.id, ticket.encryptedExecutorId.c_str(), ticket.executorId.c_str(), ticket.newModelVersion.c_str());
        
        // ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        // if (ret) {
        //     LOG_ERROR("function:completePackageSendTicket 更新工单执行记录失败！失败原因：%s", mysql_error(conn));
        //     mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        //     return false;
        // }
        
        // 3. 为加密执行者添加待办
        snprintf(sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values('%s','执行人',%d);", 
            ticket.encryptedExecutorId.c_str(), ticket.id);
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completePackageSendTicket 插入user_multi_role表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        // 4. 删除当前执行者的待办
        snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '执行人';", 
            ticket.id, ticket.executorId.c_str());
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completePackageSendTicket 删除user_multi_role表失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        // 5. 更新package_send表的封装相关信息
        if (ticket.ticketType == "版本迭代+交付发送") {
            // 插入model和newModelVersion获取自增id
            snprintf(sql, SQL_MAX, "INSERT INTO model_version(version,model) VALUES('%s', '%s');", ticket.newModelVersion.c_str(),ticket.model.c_str());	
            ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
            if (ret) {
                LOG_ERROR("function:插入model和newModelVersion获取自增id 添加模型版本失败！失败原因：%s", mysql_error(conn));
                return false;
            }
                // 获取插入的自增ID
            unsigned long long insertedId = mysql_insert_id(conn);
            LOG_INFO("新插入的model_version记录ID: %llu", insertedId);

            snprintf(sql, SQL_MAX,
                "UPDATE package_send SET new_model_version_id = %llu WHERE work_order_id = %d",
                insertedId, ticket.id);
            ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
            if (ret) {
                LOG_ERROR("function:completePackageSendTicket 更新package_send表失败！失败原因：%s", mysql_error(conn));
                return false;
            }
        }
        //     LOG_ERROR("function:completePackageSendTicket 不支持的工单类型: %s", ticket.ticketType.c_str());
        //     mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        //     return false;
        // }
        
        // ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        // if (ret) {
        //     LOG_ERROR("function:completePackageSendTicket 更新package_send表失败！失败原因：%s", mysql_error(conn));
        //     mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        //     return false;
        // }
        
        // 提交事务
        if (mysql_real_query(conn, "COMMIT", strlen("COMMIT"))) {
            LOG_ERROR("function:completePackageSendTicket 提交事务失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        LOG_INFO("function:completePackageSendTicket 版本迭代+交付发送工单封装步骤完成成功，工单ID: %d", ticket.id);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("function:completePackageSendTicket 处理过程中发生异常: %s", e.what());
        mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
}

bool TicketDAO::completePackageSendEncryptedTicket(const TicketPackage& ticket)
{
    MYSQL* conn = getConnection();
    if (!conn) {
        LOG_ERROR("function:completePackageSendEncryptedTicket 获取数据库连接失败");
        return false;
    }
    
    // 开始事务
    if (mysql_real_query(conn, "START TRANSACTION", strlen("START TRANSACTION"))) {
        LOG_ERROR("function:completePackageSendEncryptedTicket 开始事务失败！失败原因：%s", mysql_error(conn));
        return false;
    }
    
    char sql[SQL_MAX];
    int ret;
    // 获取当前时间
    std::time_t now = std::time(nullptr);
    char timeStr[20];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", std::gmtime(&now));
    std::string completedTime = timeStr;
    
    try {
        // 1. 更新status_todo状态待发送
        snprintf(sql, SQL_MAX, 
            "UPDATE work_order SET status_todo = '待发送', encrypted_remark = '%s' WHERE id = %d"
            , ticket.remark.c_str(), ticket.id);
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completePackageSendEncryptedTicket 更新工单状态失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        // 2. 插入工单执行记录的加密信息
        snprintf(sql, SQL_MAX,
            "INSERT INTO work_order_executor (work_order_id, executor_id, create_id,  status,create_at) VALUES (%d, '%s', '%s', '发送', NOW())", 
            ticket.id, ticket.sendExecutorId.c_str(), ticket.executorId.c_str());
        
        ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:completePackageSendEncryptedTicket 更新工单执行记录失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        if(ticket.encrypted){
            // UPDATE `package_send` SET `encryption_key` = 'A53000000004,A53000000005,A53000000006', `auth_id` = '2025032672' WHERE `work_order_id` = 38
            snprintf(sql, SQL_MAX,
            "UPDATE `package_send` SET `encryption_key` = '%s', `auth_id` = '%s', `is_encrypted` = '1' WHERE `work_order_id` = %d",
            ticket.dongles.c_str(), ticket.license.c_str(), ticket.id);
            ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
            if (ret) {
                LOG_ERROR("function:completePackageSendEncryptedTicket 更新package_send表失败！失败原因：%s", mysql_error(conn));
                mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
                return false;
            }
        }
        
        
        // 提交事务
        if (mysql_real_query(conn, "COMMIT", strlen("COMMIT"))) {
            LOG_ERROR("function:completePackageSendEncryptedTicket 提交事务失败！失败原因：%s", mysql_error(conn));
            mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        LOG_INFO("function:completePackageSendEncryptedTicket 工单加密完成成功，工单ID: %d", ticket.id);
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("function:completePackageSendEncryptedTicket 处理过程中发生异常: %s", e.what());
        mysql_real_query(conn, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
}

TicketDAO::~TicketDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
