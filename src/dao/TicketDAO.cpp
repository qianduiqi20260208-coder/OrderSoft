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

bool TicketDAO::createConcreteTicket(Ticket &ticket)
{
        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;

    //往具体工单表中插入数据
    //判断是什么类型的工单
    if(ticket.ticketType == "问题复现")
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
        snprintf(local_sql, SQL_MAX, "INSERT INTO version_iteration(work_order_id,coordination_id,update_content,packaging_requirements,interface_changed,remarks) "
        "VALUES(%d,'%s','%s', '%s',%d,'%s');",ticket.Ticket::id,tv.coordinationId.c_str(), tv.updateNote.c_str(),tv.packRequirement.c_str(),tv.interfaceChanged,tv.remark.c_str());
    }else if(ticket.ticketType == "交付发送")
    {
        TicketDelivery& td = dynamic_cast<TicketDelivery&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO delivery_send(work_order_id,target_customer,validated_by_cae,sensitive_info) "
        "VALUES(%d,'%s',%d, '%s');", ticket.Ticket::id,td.targetClient.c_str(), td.validatedByCAE,td.sensitiveInfo.c_str());
    }else if(ticket.ticketType == "直接封装+发送")
    {
        TicketPackage& tp = dynamic_cast<TicketPackage&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO package_send(work_order_id,coordination_id,update_content,packaging_requirements,interface_changed,target_customer,validated_by_cae,sensitive_info,remarks) "
        "VALUES(%d,'%s','%s','%s',%d,'%s',%d,'%s','%s');", ticket.Ticket::id,tp.coordinationId.c_str(), tp.updateNote.c_str(),tp.packRequirement.c_str(),tp.interfaceChanged,tp.targetClient.c_str(),tp.validatedByCAE,tp.sensitiveInfo.c_str(),tp.remark.c_str());        
    }else if(ticket.ticketType == "功能开发")
    {
        TicketFeature& tf = dynamic_cast<TicketFeature&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO function_development(work_order_id,description_create,description_completed) "
        "VALUES(%d,'%s','%s');", ticket.Ticket::id,tf.featureInit.c_str(),tf.featureFinal.c_str());
    }else if(ticket.ticketType == "其他")
    {
        TicketOther& to = dynamic_cast<TicketOther&>(ticket);
        snprintf(local_sql, SQL_MAX, "INSERT INTO other_work_order(work_order_id,description) "
        "VALUES(%d,'%s');", ticket.Ticket::id,to.description.c_str());
    }
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));

	if (local_ret) {
		LOG_ERROR("function:createConcreteTicket 插入具体的工单表失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}
    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        LOG_ERROR("function:createConcreteTicket 事务提交失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }
    return true;
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
            LOG_ERROR("function:completeConcreteTicket 修改issue_reproduction表失败！失败原因：%s", mysql_store_result(conn));
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
            LOG_ERROR("function:completeConcreteTicket 修改version_iteration表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else if(ticket.ticketType == "交付发送"){
        //去产品授权表里根据外壳号以及授权码找出唯一的产品授权ID
        const TicketDelivery& tmp = dynamic_cast<const TicketDelivery&>(ticket);
        //判断是否加密，然后分开处理
        if(tmp.encrypted)
        {
            snprintf(local_sql, SQL_MAX, "update delivery_send set is_encrypted = %d,shell_code = '%s',authorization_id = (select id from product_authorization where authorization_code = '%s' and encryption_key"
                " = '%s'),remarks = '%s' where work_order_id = %d;", tmp.encrypted,tmp.dongleId.c_str(),tmp.licenseId.c_str(),tmp.dongleId.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        }else{
            snprintf(local_sql, SQL_MAX, "update delivery_send set is_encrypted = 0, remarks = '%s' where work_order_id = %d;", tmp.remark.c_str(), tmp.Ticket::id);
        }

        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改delivery_send表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        
    }else if(ticket.ticketType == "直接封装+发送"){
        
        const TicketPackage& tmp = dynamic_cast<const TicketPackage&>(ticket);
        if(tmp.encrypted)
        {
            snprintf(local_sql, SQL_MAX, "update package_send set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),is_encrypted = %d,encryption_key = '%s' ,product_authorization_id = (select id from product_authorization where authorization_code = '%s' and encryption_key"
            " ='%s'),remarks = '%s' where work_order_id = %d;", tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.encrypted,tmp.dongle.c_str(),tmp.license.c_str(),tmp.dongle.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        }else{
            snprintf(local_sql, SQL_MAX, "update package_send set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),is_encrypted = 0 where work_order_id = %d;", tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.Ticket::id);
        }

        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改package_send表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "功能开发"){
        const TicketFeature& tmp = dynamic_cast<const TicketFeature&>(ticket);
        snprintf(local_sql, SQL_MAX, "update function_development set description_completed = '%s',new_model_version_id = (select id from model_version where model = '%s' and version = '%s' ) where work_order_id = %d;",tmp.featureFinal.c_str(),tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        printf("sql:%s\n",local_sql);
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改function_development表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "其他"){
        const TicketOther& tmp = dynamic_cast<const TicketOther&>(ticket);
        snprintf(local_sql, SQL_MAX, "update other_work_order set remarks = '%s' where work_order_id = %d;",tmp.remark.c_str(),tmp.Ticket::id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:completeConcreteTicket 修改other_work_order表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }

    //删除自己在流程中的位置
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '执行人' ;",ticket.id,ticket.executorId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:completeConcreteTicket 删除user_multi_role表失败！失败原因：%s", mysql_store_result(conn));
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
                LOG_ERROR("function:saveUploadFile 插入附件信息表失败！失败原因：%s", mysql_store_result(conn));
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
        LOG_ERROR("function:createTicket 事务开始失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }
    //获取模型版本ID
    snprintf(local_sql, SQL_MAX, "select id from model_version where model = '%s' and version = '%s';", ticket.model.c_str(),ticket.modelVersion.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
		 LOG_ERROR("function:createTicket 查询model_version表失败！失败原因：%s", mysql_store_result(conn));
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
		 LOG_ERROR("function:createTicket 插入work_order表数据失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //记录另一个人的待办
    //获取主键id
    ticket.id = mysql_insert_id(conn);
    snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role values(NULL,'%s','null','审批人',%d);", ticket.approverId.c_str(),ticket.id);	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));

	if (local_ret) {
		 LOG_ERROR("function:createTicket 插入user_multi_role数据失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //创建具体的工单
    return createConcreteTicket(ticket);;
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
        LOG_ERROR("function:approveTicket 事务开始失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }

    //修改工单状态
    //看优先级变量里是否有数据，没有数据表示拒绝，拒绝的话需要填写拒绝原因
    if(ticket.priorityHint == "")
    {
        snprintf(local_sql, SQL_MAX, "update work_order set status = '已退回',reject_reason = '%s' where id = %d;", ticket.rejectReason.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:createTicket 修改work_order表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else{
        snprintf(local_sql, SQL_MAX, "update work_order set status = '待分发', approved_at = NOW(),priority = '%s',dispatcher_id = '%s' where id = %d;", ticket.priorityHint.c_str(), ticket.distributorId.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:createTicket 修改work_order表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

        //记录另一个人的待办
        snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values('%s','分发人',%d);", ticket.distributorId.c_str(),ticket.id);	
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:createTicket 插入user_multi_role数据失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }

    //删除自己在流程中的位置
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '审批人' ;",ticket.id, ticket.approverId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:createTicket 删除user_multi_role数据失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        LOG_ERROR("function:approveTicket 事务提交失败!失败原因：%s", mysql_store_result(conn));
        return false;
    }

    return true;
}
bool TicketDAO::dispatchTicket(const Ticket& ticket)
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
        LOG_ERROR("function:dispatchTicket 事务开始失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }

    //看任务优先级变量里是否有数据，没有数据表示拒绝，拒绝的话需要填写拒绝原因
    if(ticket.priorityTask == "")
    {
        snprintf(local_sql, SQL_MAX, "update work_order set status = '已退回',reject_reason = '%s' where id = %d;", ticket.rejectReason.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 修改work_order表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else{
        //修改工单状态
        snprintf(local_sql, SQL_MAX, "update work_order set status = '进行中', dispatched_at = NOW(),task_priority = '%s' where id = %d;", ticket.priorityTask.c_str(),ticket.id);
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 修改work_order表失败！失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

        //记录工单执行人
        snprintf(local_sql, SQL_MAX, "insert into work_order_executor(work_order_id,executor_id,transferred_at) values(%d,'%s',NOW());", ticket.id,ticket.executorId.c_str());
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 修改work_order_executor表失败!失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

        //记录另一个人的待办
        snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values('%s','执行人',%d);",ticket.executorId.c_str(),ticket.id);	
        local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:dispatchTicket 插入user_multi_role表失败！executorId失败原因：%s", mysql_store_result(conn));
            mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }

    //删除自己的待办
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = '%s' and flow_role = '分发人' ;",ticket.id, ticket.distributorId.c_str());	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:dispatchTicket 删除user_multi_role表失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
		 LOG_ERROR("function:dispatchTicket 事务提交失败!失败原因：%s", mysql_store_result(conn));
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
        LOG_ERROR("function:completeTicket 事务开始失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }


    //修改工单状态
    snprintf(local_sql, SQL_MAX, "update work_order set status = '已完成', completed_at = NOW() where id = %d;",ticket.id);
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
		 LOG_ERROR("function:completeTicket  修改work_order表失败！失败原因：%s", mysql_store_result(conn));
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
    
        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
         return {std::shared_ptr<Ticket>()};
    }
    
    MYSQL* conn = getConnection();
    
    std::vector<std::shared_ptr<Ticket>> retVec;

    std::stringstream ss;
    ss<<"select * from work_order ";
    bool first = true;
    //根据类型的不同进行不同的处理
    std::set<std::string> intSet={"id","creator_id","model_version_id","approver_id","dispatcher_id"};

    if(!filter.empty())
    {
        ss<<" where "; //如果没有过滤条件，则查询所有
    }

    for(auto ele: filter)
    {
        if(!first) ss<<" and ";
        if(intSet.find(ele.first) !=intSet.end())
        {
            //对传入的模型数组进行处理
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
            }else{
                ss<<ele.first<<" = "<<ele.second;
            }

        }else{
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
            }else{
                // 处理逗号分隔的值
                if (ele.second.find(',') != std::string::npos) {
                    std::istringstream iss(ele.second);
                    std::string token;
                    std::vector<std::string> values;
                    while (std::getline(iss, token, ',')) {
                        // 去除前后空格
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

    ss<<" order by id desc limit "<<offset<<","<<count<<";";


    printf("sql:%s\n", ss.str().c_str());
    int local_ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    
    if (local_ret) {
        LOG_ERROR("function:selectOrderByCondition() 查询work_order表失败！失败原因：%s", mysql_store_result(conn));
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
            LOG_ERROR("function:concreteTicketList() 查询issue_reproduction表失败！失败原因：%s", mysql_store_result(conn));
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
            LOG_ERROR("function:concreteTicketList() 查询version_iteration表失败！失败原因：%s", mysql_store_result(conn));
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
            LOG_ERROR("function:concreteTicketList() 查询package_send表失败！失败原因：%s", mysql_store_result(conn));
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
            tmp->dongle =  row[11]?row[11]:"";
            tmp->license =  row[12]?row[12]:"";
            tmp->remark = (row[13]?row[13]:"");
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
            LOG_ERROR("function:concreteTicketList() 查询delivery_send表失败！失败原因：%s", mysql_store_result(conn));
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
            tmp->licenseId =  (row[7]?row[7]:"");
            tmp->remark = (row[8]?row[8]:"");
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
            LOG_ERROR("function:concreteTicketList() 查询function_development表失败！失败原因：%s", mysql_store_result(conn));
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
            LOG_ERROR("function:concreteTicketList() 查询other_work_order表失败！失败原因：%s", mysql_store_result(conn));
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

bool TicketDAO::orderTransfer(const TicketExecutor &executor)
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
        LOG_ERROR("function:orderTransfer 事务开始失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }

    snprintf(local_sql, SQL_MAX, "insert into work_order_executor values(NULL,%d,'%s',NOW(),'%s');", executor.ticketId,executor.executor[1].c_str(),executor.reason[0].c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:orderTransfer 添加work_order_executor表失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
        return false;
    }

    //记录另一个人的待办
    snprintf(local_sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values(%d,'执行人',%d);",stoi(executor.executor[1]),executor.ticketId);	
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    printf("sql:%s\n",local_sql);
    if (local_ret) {
        LOG_ERROR("function:orderTransfer 插入user_multi_role数据失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
        return false;
    }
    
    //删除自己在流程中的位置
    snprintf(local_sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d and flow_role = '执行人' ;",executor.ticketId,stoi(executor.executor[0]));	
	local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
	if (local_ret) {
		 LOG_ERROR("function:orderTransfer 删除user_multi_role数据失败！失败原因：%s", mysql_store_result(conn));
        mysql_real_query(conn, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(conn, "COMMIT",strlen("COMMIT"))) {
        LOG_ERROR("function:orderTransfer 事务提交失败！失败原因：%s", mysql_store_result(conn));
        return false;
    }

    return true;
}

unsigned long long TicketDAO::getOrderCount()
{

        // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    unsigned long long retCount = -1;

    snprintf(local_sql, SQL_MAX, "select count(*) from work_order;");
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderCount() 查询work_order表失败！失败原因：%s", mysql_store_result(conn));
        return retCount;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    if(row = mysql_fetch_row(res))
    {
        retCount = atoi(row[0]);
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
        LOG_ERROR("function:getOrderClient() 查询customer_info表失败！失败原因：%s", mysql_store_result(conn));
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
        LOG_ERROR("function:getVersionsWithPagination() 查询失败！失败原因：%s", mysql_store_result(conn));
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
        LOG_ERROR("function:getVersionsCount() 查询失败！失败原因：%s", mysql_store_result(conn));
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
       << "wo.creator_id as creatorId, "
       << "wo.created_at as createTime, "
       << "wo.type as ticketType, "
       << "wo.model, "
       << "(SELECT version FROM model_version WHERE id = wo.model_version_id) as modelVersion, "
       << "wo.status, "
       << "wo.approver_id as approverId, "
       << "wo.priority as priorityHint, "
       << "wo.dispatcher_id as distributorId, "
       << "woe.executor_id as executor_id, "
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
       << "  WHEN wo.type = '直接封装+发送' THEN ps.product_authorization_id "
       << "  ELSE NULL "
       << "END AS license, "
       << "CASE "
       << "  WHEN wo.type = '交付发送' THEN ds.authorization_id "
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
       << "LEFT JOIN work_order_executor woe ON woe.work_order_id = wo.id "
       << "LEFT JOIN issue_reproduction ir ON ir.work_order_id = wo.id "
       << "LEFT JOIN issue_reproduction_attachment ira ON ira.ticket_id = ir.work_order_id "
       << "LEFT JOIN package_send ps ON ps.work_order_id = wo.id "
       << "LEFT JOIN delivery_send ds ON ds.work_order_id = wo.id "
       << "LEFT JOIN function_development fd ON fd.work_order_id = wo.id "
       << "LEFT JOIN other_work_order owo ON owo.work_order_id = wo.id "
       << "WHERE mv.model = '" << modelName << "' "
       << "AND mv.version IS NOT NULL "
       << "AND mv.version IN " << versionInClause.str() << " "
       << "AND wo.type != '版本迭代' "
       << ") UNION ("
       << "SELECT DISTINCT "
       << "wo.id, "
       << "mv.id as mv_id, "
       << "mv.version, "
       << "mv.update_time, "
       << "wo.creator_id as creatorId, "
       << "wo.created_at as createTime, "
       << "wo.type as ticketType, "
       << "wo.model, "
       << "(SELECT version FROM model_version WHERE id = wo.model_version_id) as modelVersion, "
       << "wo.status, "
       << "wo.approver_id as approverId, "
       << "wo.priority as priorityHint, "
       << "wo.dispatcher_id as distributorId, "
       << "woe.executor_id as executor_id, "
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
       << "  WHEN wo.type = '直接封装+发送' THEN ps.product_authorization_id "
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
       << "LEFT JOIN work_order_executor woe ON woe.work_order_id = wo.id "
       << "WHERE mv.model = '" << modelName << "' "
       << "AND mv.version IS NOT NULL "
       << "AND mv.version IN " << versionInClause.str() << " "
       << "AND wo.type IN ('版本迭代', '功能开发', '直接封装+发送') "
       << ") ORDER BY id DESC;";

    int ret;
    ret = mysql_real_query(conn, ss.str().c_str(), ss.str().size());
    
    if (ret) {
        LOG_ERROR("function:getWorkOrdersWithDetailsByVersions() 查询失败！失败原因：%s", mysql_store_result(conn));
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
            LOG_ERROR("function:selectOrderStatisticsByCondition() 查询 work_order 表失败！失败原因：%s", mysql_store_result(conn));
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



TicketDAO::~TicketDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
