#include "TicketDAO.h"
#include "IniReader.h"
#include <fstream>
#include <sstream>
#include <memory>
#include "DBConnectionManager.h"

TicketDAO::TicketDAO(MYSQL *ms):mysql(ms)
{
    DBConnectionManager::getConnection(mysql);
}

bool TicketDAO::createConcreteTicket(Ticket &ticket)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    //往具体工单表中插入数据
    //判断是什么类型的工单
    if(ticket.ticketType == "问题复现")
    {
        TicketReproduce& tp = dynamic_cast<TicketReproduce&>(ticket);
        snprintf(sql, SQL_MAX, "INSERT INTO issue_reproduction(work_order_id,coordination_id,description) "
        "VALUES(%d,'%s','%s');", ticket.Ticket::id,tp.coordinationId.c_str(), tp.content.c_str());
        
        //这里需要先执行一次 避免sql被覆盖掉
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        printf("sql:%s\n",sql);
        if (ret) {
            printf("[error] function:createConcreteTicket 插入issue_reproduction失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        if(!saveUploadFile(tp))
            return false;
    }else if(ticket.ticketType == "版本迭代")
    {
        TicketVersion& tv = dynamic_cast<TicketVersion&>(ticket);
        snprintf(sql, SQL_MAX, "INSERT INTO version_iteration(work_order_id,coordination_id,update_content,packaging_requirements,interface_changed) "
        "VALUES(%d,'%s','%s', '%s',%d);",ticket.Ticket::id,tv.coordinationId.c_str(), tv.updateNote.c_str(),tv.packRequirement.c_str(),tv.interfaceChanged);
    }else if(ticket.ticketType == "交付发送")
    {
        TicketDelivery& td = dynamic_cast<TicketDelivery&>(ticket);
        snprintf(sql, SQL_MAX, "INSERT INTO delivery_send(work_order_id,target_customer,validated_by_cae,sensitive_info) "
        "VALUES(%d,'%s',%d, '%s');", ticket.Ticket::id,td.targetClient.c_str(), td.validatedByCAE,td.sensitiveInfo.c_str());
    }else if(ticket.ticketType == "直接封装+发送")
    {
        TicketPackage& tp = dynamic_cast<TicketPackage&>(ticket);
        snprintf(sql, SQL_MAX, "INSERT INTO package_send(work_order_id,coordination_id,update_content,packaging_requirements,interface_changed,target_customer,validated_by_cae,sensitive_info) "
        "VALUES(%d,'%s','%s','%s',%d,'%s',%d,'%s');", ticket.Ticket::id,tp.coordinationId.c_str(), tp.updateNote.c_str(),tp.packRequirement.c_str(),tp.interfaceChanged,tp.targetClient.c_str(),tp.validatedByCAE,tp.sensitiveInfo.c_str());        
    }else if(ticket.ticketType == "功能开发")
    {
        TicketFeature& tf = dynamic_cast<TicketFeature&>(ticket);
        snprintf(sql, SQL_MAX, "INSERT INTO function_development(work_order_id,description_create) "
        "VALUES(%d,'%s');", ticket.Ticket::id,tf.featureInit.c_str());
    }else if(ticket.ticketType == "其他")
    {
        TicketOther& to = dynamic_cast<TicketOther&>(ticket);
        snprintf(sql, SQL_MAX, "INSERT INTO other_work_order(work_order_id,description) "
        "VALUES(%d,'%s');", ticket.Ticket::id,to.description.c_str());
    }
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));

	if (ret) {
		printf("[error] function:createConcreteTicket 插入具体的工单表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}
    //事务提交
    if (mysql_real_query(mysql, "COMMIT",strlen("COMMIT"))) {
        printf("[error] function:createConcreteTicket 事务提交失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }
    return true;
}

bool TicketDAO::completeConcreteTicket(const Ticket &ticket)
{
    if(ticket.ticketType == "问题复现")
    {
        const TicketReproduce& tmp = dynamic_cast<const TicketReproduce&>(ticket);

        snprintf(sql, SQL_MAX, "update issue_reproduction set phenomenon = '%s',remarks = '%s' where work_order_id = %d;", tmp.phenomenon.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:completeConcreteTicket 修改issue_reproduction表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "版本迭代")
    {
        const TicketVersion& tmp = dynamic_cast<const TicketVersion&>(ticket);

        //根据模型以及模型版本去模型版本表里把具体的模型版本ID查出来
        snprintf(sql, SQL_MAX, "update version_iteration set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),remarks = '%s' where work_order_id = %d;",tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:completeConcreteTicket 修改version_iteration表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else if(ticket.ticketType == "交付发送"){
        //去产品授权表里根据外壳号以及授权码找出唯一的产品授权ID
        const TicketDelivery& tmp = dynamic_cast<const TicketDelivery&>(ticket);
        snprintf(sql, SQL_MAX, "update delivery_send set is_encrypted = %d,shell_code = '%s',authorization_id = (select id from product_authorization where authorization_code = '%s' and encryption_key"
            " = '%s'),remarks = '%s' where work_order_id = %d;", tmp.encrypted,tmp.dongleId.c_str(),tmp.licenseId.c_str(),tmp.dongleId.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:completeConcreteTicket 修改delivery_send表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
        
    }else if(ticket.ticketType == "直接封装+发送"){
        const TicketPackage& tmp = dynamic_cast<const TicketPackage&>(ticket);
        snprintf(sql, SQL_MAX, "update package_send set new_model_version_id = (select id from model_version where model = '%s' and version = '%s'),is_encrypted = %d,encryption_key = '%s' ,product_authorization_id = (select id from product_authorization where authorization_code = '%s' and encryption_key"
        " ='%s'),remarks = '%s' where work_order_id = %d;", tmp.model.c_str(),tmp.newModelVersion.c_str(),tmp.encrypted,tmp.dongle.c_str(),tmp.license.c_str(),tmp.dongle.c_str(),tmp.remark.c_str(),tmp.Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:completeConcreteTicket 修改package_send表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "功能开发"){
        const TicketFeature& tmp = dynamic_cast<const TicketFeature&>(ticket);
        snprintf(sql, SQL_MAX, "update function_development set description_completed = '%s' where work_order_id = %d;",tmp.featureFinal.c_str(),tmp.Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:completeConcreteTicket 修改function_development表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }else if(ticket.ticketType == "其他"){
        const TicketOther& tmp = dynamic_cast<const TicketOther&>(ticket);
        snprintf(sql, SQL_MAX, "update other_work_order set remarks = '%s' where work_order_id = %d;",tmp.remark.c_str(),tmp.Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:completeConcreteTicket 修改other_work_order表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }

    }

    //删除自己在流程中的位置
    snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d ;",ticket.id,ticket.executorId);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
	if (ret) {
		printf("[error] function:completeConcreteTicket 删除user_multi_role表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(mysql, "COMMIT",strlen("COMMIT"))) {
        std::cerr << "事务提交失败：" << mysql_error(mysql) << "\n";
        return false;
    }

    return true;
}

bool TicketDAO::saveUploadFile(const TicketReproduce &ticket)
{
    //保存文件
    IniReader config;
    if (!config.load("../../config/config.ini")) { 
        std::cerr << "无法读取 config.ini 文件\n";
        return false;
    }
    std::string relativePath = "/ticket/"+ticket.attachment.fileName;
    std::string filePath = config.getString("storage","upload_dir") + relativePath;
    std::fstream ofs(filePath,std::ios::binary | std::ios::out);
    if(!ofs.is_open())
    {
        printf("[error] function:saveUploadFile 文件打开失败!filePath:%s\n",filePath.c_str());
    }
    ofs.write(ticket.attachment.file.c_str(),ticket.attachment.file.size());
    ofs.close();

    //数据库存储
	snprintf(sql, SQL_MAX, "INSERT INTO issue_reproduction_attachment(ticket_id,file_path,file_name) "
        "VALUES(%d,'%s', '%s');", ticket.Ticket::id, relativePath.c_str(),ticket.attachment.fileName.c_str());	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:saveUploadFile 插入附件信息表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}
    return true;
}

bool TicketDAO::createTicket(Ticket &ticket)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    //启用事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:createTicket 事务开始失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }
    //获取模型版本ID
    snprintf(sql, SQL_MAX, "select id from model_version where model = '%s' and version = '%s';", ticket.model.c_str(),ticket.modelVersion.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:createTicket 查询model_version表失败！失败原因：%s\n", mysql_error(mysql));
		return false;
	}
    res = mysql_store_result(mysql);
    row = mysql_fetch_row(res);
    if(!row)
        return false;
    else
        ticket.modelVersion = row[0];
    // 释放结果集
	mysql_free_result(res);

    //往工单表中插入数据
	snprintf(sql, SQL_MAX, "INSERT INTO work_order(id,creator_id,type,model,model_version_id,approver_id) "
        "VALUES(NULL,%d,'%s', '%s', %d,%d);", ticket.creatorId, ticket.ticketType.c_str(),ticket.model.c_str(),stoi(ticket.modelVersion),ticket.approverId);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    printf("sql:%s\n",sql);
	if (ret) {
		printf("[error] function:createTicket 插入work_order表数据失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //记录另一个人的待办
    //获取主键id
    ticket.id = mysql_insert_id(mysql);
    snprintf(sql, SQL_MAX, "INSERT INTO user_multi_role values(NULL,%d,'null','审批人',%d);",ticket.approverId,ticket.id);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));

	if (ret) {
		printf("[error] function:createTicket 插入user_multi_role数据失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //创建具体的工单
    return createConcreteTicket(ticket);;
}
bool TicketDAO::approveTicket(const Ticket &ticket)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }
    //启用事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:approveTicket 事务开始失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    //修改工单状态
    snprintf(sql, SQL_MAX, "update work_order set status = '待分发', approved_at = NOW(),priority = '%s',dispatcher_id = %d where id = %d;", ticket.priorityHint.c_str(),ticket.distributorId,ticket.id);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:createTicket 修改work_order表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}
    //记录另一个人的待办
    snprintf(sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values(%d,'分发人',%d);",ticket.distributorId,ticket.id);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
	if (ret) {
		printf("[error] function:createTicket 插入user_multi_role数据失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //删除自己在流程中的位置
    snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d ;",ticket.id,ticket.approverId);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
	if (ret) {
		printf("[error] function:createTicket 删除user_multi_role数据失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //事务提交
    if (mysql_real_query(mysql, "COMMIT",strlen("COMMIT"))) {
        printf("[error] function:approveTicket 事务提交失败!失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}
bool TicketDAO::dispatchTicket(const Ticket& ticket)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false; 
    }
    //启用事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:dispatchTicket 事务开始失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    //修改工单状态
    snprintf(sql, SQL_MAX, "update work_order set status = '进行中', dispatched_at = NOW(),task_priority = '%s' where id = %d;", ticket.priorityTask.c_str(),ticket.id);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:dispatchTicket 修改work_order表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //记录工单执行人
    snprintf(sql, SQL_MAX, "insert into work_order_executor(work_order_id,executor_id,transferred_at) values(%d,%d,NOW());", ticket.id,ticket.executorId);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:dispatchTicket 修改work_order_executor表失败!失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //记录另一个人的待办
    snprintf(sql, SQL_MAX, "INSERT INTO user_multi_role(user_id,flow_role,work_order_id) values(%d,'执行人',%d);",ticket.executorId,ticket.id);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
	if (ret) {
		printf("[error] function:dispatchTicket 插入user_multi_role表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    //删除自己在流程中的位置
    snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d ;",ticket.id,ticket.distributorId);	
	ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
	if (ret) {
		printf("[error] function:dispatchTicket 删除user_multi_role表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

        //事务提交
    if (mysql_real_query(mysql, "COMMIT",strlen("COMMIT"))) {
		printf("[error] function:dispatchTicket 事务提交失败!失败原因：%s\n", mysql_error(mysql));
        return false;
    }
    return true;
}
bool TicketDAO::completeTicket(const Ticket &ticket)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }
        //启用事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:completeTicket 事务开始失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }


    //修改工单状态
    snprintf(sql, SQL_MAX, "update work_order set status = '已完成', completed_at = NOW() where id = %d;",ticket.id);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:completeTicket  修改work_order表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
		return false;
	}

    return completeConcreteTicket(ticket);
}

TicketDAO::~TicketDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
