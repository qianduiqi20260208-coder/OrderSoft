#include "TicketDAO.h"
#include "IniReader.h"
#include <fstream>
#include <sstream>
#include <memory>
#include "DBConnectionManager.h"
#include "UserDAO.h"
#include <set>

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
    snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d and flow_role = '执行人' ;",ticket.id,ticket.executorId);	
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
    //看优先级变量里是否有数据，没有数据表示拒绝，拒绝的话需要填写拒绝原因
    if(ticket.priorityHint == "")
    {
        snprintf(sql, SQL_MAX, "update work_order set status = '已退回',reject_reason = '%s' where id = %d;", ticket.rejectReason.c_str(),ticket.id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:createTicket 修改work_order表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK",strlen("ROLLBACK"));
            return false;
        }
    }else{
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
    }

    //删除自己在流程中的位置
    snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d and flow_role = '审批人' ;",ticket.id,ticket.approverId);	
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
    snprintf(sql, SQL_MAX, "delete from user_multi_role where work_order_id = %d and user_id = %d and flow_role = '分发人' ;",ticket.id,ticket.distributorId);	
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

std::vector<std::shared_ptr<Ticket>> TicketDAO::ticketList(int offset, int count)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {std::shared_ptr<Ticket>()};
    }

    std::vector<std::shared_ptr<Ticket>> retVec;

    snprintf(sql, SQL_MAX, "select * from work_order order by id desc limit %d,%d;",offset,count);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:ticketList() 查询work_order表失败！失败原因：%s\n", mysql_error(mysql));
        return {std::shared_ptr<Ticket>()};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::shared_ptr<Ticket> tmp;
        //判断工单的类型
        if(std::string(row[3]) == "问题复现")
        {
            tmp = std::make_shared<TicketReproduce>();
        }else if(std::string(row[3]) == "版本迭代")
        {   
            tmp = std::make_shared<TicketVersion>();
        }else if(std::string(row[3]) == "直接封装+发送")
        {
            tmp = std::make_shared<TicketPackage>();
        }else if(std::string(row[3]) == "交付发送")
        {
            tmp = std::make_shared<TicketDelivery>();
        }else if(std::string(row[3]) == "功能开发")
        {
            tmp = std::make_shared<TicketFeature>();
        }else if(std::string(row[3]) == "其他")
        {
            tmp = std::make_shared<TicketOther>();
        }
        //封装一些共有的信息
        tmp->id = atoi(row[0]);
        tmp->creatorId = atoi(row[1]);
        tmp->createTime = row[2];
        tmp->ticketType = row[3];
        tmp->model = row[4];
        tmp->modelVersion = row[5];
        tmp->status = row[6];
        tmp->approverId = atoi(row[7]);

        tmp->priorityHint = (row[8]?row[8]:"");
        tmp->distributorId = atoi(row[9]?row[9]:"-1");
        tmp->approvedTime = row[10]?row[10]:"";
        tmp->priorityTask = row[11]?row[11]:"";
        tmp->distributedTime = row[12]?row[12]:"";
        tmp->completedTime = row[13]?row[13]:"";
        tmp->rejectReason = row[14]?row[14]:"";

        //查询模型版本
        if(tmp->modelVersion !="")
            tmp->modelVersion = UserDAO::queryModelVersion(stoi(tmp->modelVersion));
        //封装工单执行人
        tmp->executor = UserDAO::queryTicketExecutor(tmp->id);
        
        //封装一些私有的信息
        concreteTicketList(tmp->id,tmp,retVec);
    }
    mysql_free_result(res);

    return retVec;
}

std::vector<std::shared_ptr<Ticket>> TicketDAO::selectOrderByCondition(const std::map<std::string, std::string> filter,int offset, int count)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {std::shared_ptr<Ticket>()};
    }
    
    std::vector<std::shared_ptr<Ticket>> retVec;

    std::stringstream ss;
    ss<<"select * from work_order ";
    bool first = true;
    //根据类型的不同进行不同的处理
    std::set<std::string> intSet={"id","creator_id","model_version_id","approver_id","dispatcher_id"};

    if(!filter.empty())
    {
        ss<<"where ";
    }
    for(auto ele: filter)
    {
        if(!first) ss<<" and ";
        if(intSet.find(ele.first) !=intSet.end())
        {
            ss<<ele.first<<" = "<<ele.second;
        }else{
            ss<<ele.first<<" = '"<<ele.second<<"'";
        }
        first = false;
    }

    ss<<" order by id desc limit "<<offset<<","<<count<<";";


    printf("sql:%s\n", ss.str().c_str());
    ret = mysql_real_query(mysql, ss.str().c_str(), ss.str().size());
    
    if (ret) {
        printf("[error] function:selectOrderByCondition() 查询work_order表失败！失败原因：%s\n", mysql_error(mysql));
        return {std::shared_ptr<Ticket>()};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::shared_ptr<Ticket> tmp;
        //判断工单的类型
        if(std::string(row[3]) == "问题复现")
        {
            tmp = std::make_shared<TicketReproduce>();
        }else if(std::string(row[3]) == "版本迭代")
        {   
            tmp = std::make_shared<TicketVersion>();
        }else if(std::string(row[3]) == "直接封装+发送")
        {
            tmp = std::make_shared<TicketPackage>();
        }else if(std::string(row[3]) == "交付发送")
        {
            tmp = std::make_shared<TicketDelivery>();
        }else if(std::string(row[3]) == "功能开发")
        {
            tmp = std::make_shared<TicketFeature>();
        }else if(std::string(row[3]) == "其他")
        {
            tmp = std::make_shared<TicketOther>();
        }
        //封装一些共有的信息
        tmp->id = atoi(row[0]);
        tmp->creatorId = atoi(row[1]);
        tmp->createTime = row[2];
        tmp->ticketType = row[3];
        tmp->model = row[4];
        tmp->modelVersion = row[5];
        tmp->status = row[6];
        tmp->approverId = atoi(row[7]);

        tmp->priorityHint = (row[8]?row[8]:"");
        tmp->distributorId = atoi(row[9]?row[9]:"-1");
        tmp->approvedTime = row[10]?row[10]:"";
        tmp->priorityTask = row[11]?row[11]:"";
        tmp->distributedTime = row[12]?row[12]:"";
        tmp->completedTime = row[13]?row[13]:"";
        tmp->rejectReason = row[14]?row[14]:"";

        //查询模型版本
        if(tmp->modelVersion !="")
            tmp->modelVersion = UserDAO::queryModelVersion(stoi(tmp->modelVersion));
        //封装工单执行人
        tmp->executor = UserDAO::queryTicketExecutor(tmp->id);
        
        //封装一些私有的信息
        concreteTicketList(tmp->id,tmp,retVec);
    }
    
    return retVec;
}

void TicketDAO::concreteTicketList(int work_order_id, std::shared_ptr<Ticket> vecElement,std::vector<std::shared_ptr<Ticket>> & retVec)
{
    //判断具体工单的类型
    if(vecElement->ticketType == "问题复现")
    {
        auto tmp = std::static_pointer_cast<TicketReproduce>(vecElement);
        snprintf(sql, SQL_MAX, "select * from issue_reproduction where work_order_id = %d;",tmp->Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:concreteTicketList() 查询issue_reproduction表失败！失败原因：%s\n", mysql_error(mysql));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(mysql);
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
        snprintf(sql, SQL_MAX, "select * from version_iteration where work_order_id = %d;",tmp->Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:concreteTicketList() 查询version_iteration表失败！失败原因：%s\n", mysql_error(mysql));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(mysql);
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
        if(tmp->newModelVersion !="")
            tmp->newModelVersion = UserDAO::queryModelVersion(stoi(tmp->newModelVersion));

        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "直接封装+发送"){
        auto tmp = std::static_pointer_cast<TicketPackage>(vecElement);

        snprintf(sql, SQL_MAX, "select * from package_send where work_order_id = %d;",tmp->Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:concreteTicketList() 查询package_send表失败！失败原因：%s\n", mysql_error(mysql));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(mysql);
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
            tmp->newModelVersion = UserDAO::queryModelVersion(stoi(tmp->newModelVersion));
        if(tmp->license != "")
            tmp->license = UserDAO::queryProductAuthorization(stoi(tmp->license));
        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "交付发送"){
        auto tmp = std::static_pointer_cast<TicketDelivery>(vecElement);
        snprintf(sql, SQL_MAX, "select * from delivery_send where work_order_id = %d;",tmp->Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:concreteTicketList() 查询delivery_send表失败！失败原因：%s\n", mysql_error(mysql));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(mysql);
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
        snprintf(sql, SQL_MAX, "select * from function_development where work_order_id = %d;",tmp->Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:concreteTicketList() 查询function_development表失败！失败原因：%s\n", mysql_error(mysql));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(mysql);
        MYSQL_ROW row;
        if(row = mysql_fetch_row(res))
        {
            tmp->featureInit = (row[2]?row[2]:"");
            tmp->featureFinal = (row[3]?row[3]:"");
        }
        mysql_free_result(res);
        retVec.push_back(tmp);
    }else if(vecElement->ticketType == "其他"){
        auto tmp = std::static_pointer_cast<TicketOther>(vecElement);
        snprintf(sql, SQL_MAX, "select * from other_work_order where work_order_id = %d;",tmp->Ticket::id);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:concreteTicketList() 查询other_work_order表失败！失败原因：%s\n", mysql_error(mysql));
            retVec.push_back(std::shared_ptr<Ticket>());
            return;
        }
        MYSQL_RES* res = mysql_store_result(mysql);
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
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    snprintf(sql, SQL_MAX, "insert into work_order_executor values(NULL,%d,'%s',NOW(),'%s');", executor.ticketId,executor.executor[1].c_str(),executor.reason[0].c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        std::cout<<"[error] function:orderTransfer 添加work_order_executor表失败！失败原因：%s\n";
        return false;
    }
    return true;
}

TicketDAO::~TicketDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
