#include "UserDAO.h"
#include "DBConnectionManager.h"
#include <sstream>
#include <fstream>
#include "IniReader.h"
#include "Logger.h"

UserDAO::UserDAO() : mysql(nullptr)
{
    // 使用连接池，不需要初始化mysql指针
}

UserDAO::UserDAO(MYSQL *ms):mysql(ms)
{
    LOG_WARNING("UserDAO: 使用已废弃的构造函数，建议使用连接池");
    DBConnectionManager::getConnection(mysql);
}

std::vector<User> UserDAO::getUser()
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return {};
    }
    
    MYSQL* mysql = getConnection();
    std::vector<User> userVec;
    
    char local_sql[SQL_MAX];
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    int local_ret;

    //获取所有用户
    snprintf(local_sql, SQL_MAX, "select * from user;");
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
		LOG_ERROR("function:getUser() 查询user表失败！失败原因：%s", mysql_error(mysql));
		return {};
	}
    local_res = mysql_store_result(mysql);
    while(local_row = mysql_fetch_row(local_res))
    {
        User user;
        user.jobNumber = atoi(local_row[1]);
        user.fullName = (local_row[3] == nullptr ? "":local_row[3]);

        user.lastLoginTime = (local_row[10] == nullptr ? "":local_row[10]);
        user.isOnline = atoi(local_row[11]);
        if(local_row[12]!=nullptr)
            user.lastUsedModel = atoi(local_row[12]);
        //查询用户负责的模型
        snprintf(local_sql, SQL_MAX, "select DISTINCT model from user_model where username=%d;",user.jobNumber);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUser() 查询user_model表失败！失败原因：%s", mysql_error(mysql));
            return {};
        }
        MYSQL_RES* local_res2 = mysql_store_result(mysql);
        MYSQL_ROW local_row2;
        std::vector<std::string> userModelVec;
        while(local_row2 = mysql_fetch_row(local_res2))
        {
            userModelVec.push_back(local_row2[0]);
        }
        user.responsibleModel = userModelVec;
        mysql_free_result(local_res2);

        // 查出用户身份
        snprintf(local_sql, SQL_MAX, "select role from user_multi_role where user_id=%d and role != 'null';",user.jobNumber);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUser() 查询user_multi_role表失败！失败原因：%s", mysql_error(mysql));
            return {};
        }
        MYSQL_RES* local_res3 = mysql_store_result(mysql);
        MYSQL_ROW local_row3;
        std::vector<std::string> userRoleVec;
        while(local_row3 = mysql_fetch_row(local_res3))
        {
            userRoleVec.push_back(std::string(local_row3[0]));
        }
        user.roleVec = userRoleVec;
        mysql_free_result(local_res3);

        // 查出工单流程中的角色
        snprintf(local_sql, SQL_MAX, "select flow_role from user_multi_role where user_id = %d and role = 'null'  and work_order_id is null ;",user.jobNumber);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUser() 查询user_multi_role表失败！失败原因：%s", mysql_error(mysql));
            return {};
        }
        MYSQL_RES* local_res4 = mysql_store_result(mysql);
        MYSQL_ROW local_row4;
        std::vector<std::string> flowRoleVec;
        while(local_row4 = mysql_fetch_row(local_res4))
        {
            flowRoleVec.push_back(std::string(local_row4[0]));
        }
        user.flowRoleVec = flowRoleVec;
        mysql_free_result(local_res4);

        userVec.push_back(user);
    }

    // 释放结果集
	mysql_free_result(local_res);


    return userVec;
}

std::vector<std::string> UserDAO::getUserResponsibleModels(int userId)
{
    std::vector<std::string> responsibleModels;

    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:getUserResponsibleModels() 获取数据库连接失败");
        return responsibleModels;
    }
    
    MYSQL* conn = getConnection();

    // 参数验证
    if (userId <= 0) {
        LOG_ERROR("function:getUserResponsibleModels() 无效的用户ID：%d", userId);
        return responsibleModels;
    }

    // 查询用户负责的模型
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX, "SELECT model FROM user_model WHERE username = %d", userId);

    //  LOG_DEBUG("getUserResponsibleModels SQL: %s\n", local_sql);

    int local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getUserResponsibleModels() 查询user_model表失败！失败原因：%s", mysql_error(conn));
        return responsibleModels;
    }

    MYSQL_RES* local_res = mysql_store_result(conn);
    if (!local_res) {
        LOG_ERROR("function:getUserResponsibleModels() mysql_store_result失败：%s", mysql_error(conn));
        return responsibleModels;
    }

    MYSQL_ROW local_row;
    while ((local_row = mysql_fetch_row(local_res)) != nullptr) {
        if (local_row[0]) {  // 确保模型名称不为空
            std::string modelName = local_row[0];
            responsibleModels.push_back(modelName);
            //  LOG_DEBUG("用户 %d 负责模型: %s\n", userId, modelName.c_str());
        }
    }

    mysql_free_result(local_res);

    //  LOG_INFO("function:getUserResponsibleModels() 用户 %d 共负责 %zu 个模型\n",
    //        userId, responsibleModels.size());

    return responsibleModels;
}


std::vector<std::shared_ptr<Ticket>> UserDAO::getUserOrder(int jobNumber)
{
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {std::shared_ptr<Ticket>()};
    }
    MYSQL* mysql = getConnection();

    char local_sql[SQL_MAX];
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    int local_ret;

    std::vector<std::shared_ptr<Ticket>> userTicketVec;
    std::vector<int> ticketID;

    //查询用户所需要处理的全部工单ID
    snprintf(local_sql, SQL_MAX, "select work_order_id from user_multi_role where user_id = %d and role = 'null' and work_order_id is not null;",jobNumber);
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getUserOrder() 查询user_multi_role表失败！失败原因：%s", mysql_error(mysql));
        return {std::shared_ptr<Ticket>()};
    }
    local_res = mysql_store_result(mysql);
    while(local_row = mysql_fetch_row(local_res))
    {
        ticketID.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    //另一种形式的查询成功判断
    if(ticketID.size() == 0)
        return {};

    //根据全部工单ID去工单表里筛选出全部的工单 使用IN实现批量查找
    std::stringstream ss;
    ss<<"select * from work_order where id in(";
    for(int i=0;i<ticketID.size();i++)
    {
        ss<<ticketID[i];
        if(i<ticketID.size()-1)
            ss<<",";
    }
    ss<<") order by id desc;";
    snprintf(local_sql, SQL_MAX, ss.str().c_str());
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    printf("getUserOrder() sql:%s\n",local_sql);
    if (local_ret) {
        LOG_ERROR("function:getUserOrder() 查询work_order表失败！失败原因：%s", mysql_error(mysql));
        return {std::shared_ptr<Ticket>()};
    }
    local_res = mysql_store_result(mysql);
    extern std::map<int,std::string> id_name;
    while(local_row = mysql_fetch_row(local_res))
    {
        std::shared_ptr<Ticket> sp = getUserConcreteOrder(local_row[3],atoi(local_row[0]));
        if (!sp) {
            LOG_ERROR("getUserConcreteOrder返回空指针，跳过该工单");
            continue; // 或其他错误处理逻辑
        }
        //通用的插入代码
        {
            sp->id = atoi(local_row[0]);
            sp->creatorId = id_name[atoi(local_row[1])];
            sp->createTime = local_row[2];
            sp->ticketType = local_row[3];
            sp->model = local_row[4];
            sp->modelVersion = local_row[5];
            sp->status = local_row[6];
            sp->approverId = id_name[atoi(local_row[7])];
            sp->priorityHint = (local_row[8] == nullptr?"":local_row[8]);
            sp->distributorId = id_name[(local_row[9]?atoi(local_row[9]):0)];
            sp->approvedTime = (local_row[10] == nullptr?"":local_row[10]);
            sp->priorityTask =(local_row[11] == nullptr?"":local_row[11]);
            sp->distributedTime = (local_row[12] == nullptr?"":local_row[12]);
        }

        //查询模型版本
        if(sp->modelVersion != "")
            sp->modelVersion = queryModelVersion(stoi(sp->modelVersion));
        
        //封装执行人信息
        sp->executor = queryTicketExecutor(sp->id);
        userTicketVec.push_back(sp);
        
    }
    mysql_free_result(local_res); 

    return userTicketVec;
}

std::vector<std::pair<int, std::string>> UserDAO::getOrderApprover_()
{
    std::vector<std::pair<int, std::string>> retVec;
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {};
    }
    MYSQL* mysql = getConnection();
    
    // 联表查出审批人工号和姓名
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX, 
        "SELECT DISTINCT umr.user_id, u.real_name "
        "FROM user_multi_role umr "
        "JOIN user u ON umr.user_id = u.username "
        "WHERE umr.work_order_id IS NULL AND umr.flow_role = '审批人';"
    );
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderApprover() 查询user_multi_role/user表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        int jobNumber = atoi(local_row[0]);
        std::string realName = local_row[1] ? local_row[1] : "";
        retVec.push_back(std::make_pair(jobNumber, realName));
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<int> UserDAO::getOrderApprover()
{
    std::vector<int> retVec;
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {INT_MAX};
    }
    MYSQL* mysql = getConnection();
    
    // 查出用户身份
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX, "select DISTINCT user_id from user_multi_role where work_order_id is null and flow_role = '审批人';");
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderApprover() 查询user_multi_role表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    std::vector<std::string> userRoleVec;
    while(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<int> UserDAO::getOrderDispatcher()
{
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {INT_MAX};
    }
    MYSQL* mysql = getConnection();

    std::vector<int> retVec;

    // 查出用户身份
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX, "select DISTINCT user_id from user_multi_role where work_order_id is null and flow_role = '分发人';");
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderDispatcher() 查询user_multi_role表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    std::vector<std::string> userRoleVec;
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::pair<int, std::string>> UserDAO::getOrderDispatcher_()
{
    std::vector<std::pair<int, std::string>> retVec;
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {};
    }
    MYSQL* mysql = getConnection();

    // 联表查出分发人工号和姓名
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX,
        "SELECT DISTINCT umr.user_id, u.real_name "
        "FROM user_multi_role umr "
        "JOIN user u ON umr.user_id = u.username "
        "WHERE umr.work_order_id IS NULL AND umr.flow_role = '分发人';"
    );
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderDispatcher_() 查询user_multi_role/user表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        int jobNumber = atoi(local_row[0]);
        std::string realName = local_row[1] ? local_row[1] : "";
        retVec.push_back(std::make_pair(jobNumber, realName));
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<int> UserDAO::getOrderExecutor()
{
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {INT_MAX};
    }
    MYSQL* mysql = getConnection();

    std::vector<int> retVec;

    // 查出用户身份
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX, "select DISTINCT user_id from user_multi_role where work_order_id is null and flow_role = '执行人';");
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderExecutor() 查询user_multi_role表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    std::vector<std::string> userRoleVec;
    while(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::pair<int, std::string>> UserDAO::getOrderExecutor_()
{
    std::vector<std::pair<int, std::string>> retVec;
    //检查数据库连接状态
    if(!ensureConnection())
    {
        return {};
    }
    MYSQL* mysql = getConnection();

    // 联表查出执行人工号和姓名
    char local_sql[SQL_MAX];
    snprintf(local_sql, SQL_MAX,
        "SELECT DISTINCT umr.user_id, u.real_name "
        "FROM user_multi_role umr "
        "JOIN user u ON umr.user_id = u.username "
        "WHERE umr.work_order_id IS NULL AND umr.flow_role = '执行人';"
    );
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getOrderExecutor_() 查询user_multi_role/user表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        int jobNumber = atoi(local_row[0]);
        std::string realName = local_row[1] ? local_row[1] : "";
        retVec.push_back(std::make_pair(jobNumber, realName));
    }
    mysql_free_result(local_res);

    return retVec;
}

UserDAO::~UserDAO()
{
    DBConnectionManager::closeConnection(mysql);
}

bool UserDAO::updatePassword(const std::string& userId, const std::string& oldPassword, const std::string& newPassword)
{
    if (!ensureConnection())
    {
        return false;
    }
    MYSQL* mysql = getConnection();

    // 校验原密码
    snprintf(sql, SQL_MAX, "SELECT password FROM user WHERE username = '%s';", userId.c_str());
    int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        LOG_ERROR("function:updatePassword() 查询原密码失败！原因：%s", mysql_error(mysql));
        return false;
    }
    MYSQL_RES* res = mysql_store_result(mysql);
    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row || oldPassword != std::string(row[0])) {
        mysql_free_result(res);
        return false; // 原密码不正确
    }
    mysql_free_result(res);

    // 更新新密码
    snprintf(sql, SQL_MAX, "UPDATE user SET password = '%s' WHERE username = '%s';",
             newPassword.c_str(), userId.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret)
    {
        LOG_ERROR("function:updatePassword() 更新用户密码失败！失败原因：%s", mysql_error(mysql));
        return false;
    }

    return true;
}

std::shared_ptr<Ticket> UserDAO::getUserConcreteOrder(std::string ticketType, int workOrderId)
{
    
    MYSQL* mysql = getConnection();
    std::shared_ptr<Ticket> sp;
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    //查询各自的表
    if(ticketType == "问题复现")
    {
        std::shared_ptr<TicketReproduce> tmp = std::make_shared<TicketReproduce>();

        snprintf(local_sql, SQL_MAX, "select * from issue_reproduction where work_order_id = %d;",workOrderId);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUserOrder() 查询issue_reproduction表失败！失败原因：%s", mysql_error(mysql));
            return {std::shared_ptr<Ticket>()};
        }
        //保存ticketId的值，用来下载附件
        tmp->ticketId = workOrderId;

        local_res = mysql_store_result(mysql);
        while(local_row = mysql_fetch_row(local_res))
        {
            tmp->coordinationId = local_row[2];
            tmp->content = local_row[3];
        }
        mysql_free_result(local_res);
        //保存需要下载的附件本体 另外加上对于下载失败的处理
        if(!downloadAttachment(tmp))
        {
            return {std::shared_ptr<Ticket>()};
        }else{
            sp = tmp;
        }

    }else if(ticketType == "版本迭代")
    {
        std::shared_ptr<TicketVersion> tmp = std::make_shared<TicketVersion>();

        snprintf(local_sql, SQL_MAX, "select * from version_iteration where work_order_id = %d;",workOrderId);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUserOrder() 查询version_iteration表失败！失败原因：%s", mysql_error(mysql));
            return {std::shared_ptr<Ticket>()};
        }
        local_res = mysql_store_result(mysql);
        while(local_row = mysql_fetch_row(local_res))
        {
            tmp->coordinationId = local_row[2];
            tmp->updateNote = local_row[3];
            tmp->packRequirement = local_row[4];
            tmp->interfaceChanged = atoi(local_row[5]);
            tmp->remark = (local_row[7]?local_row[7]:"");
        }
        mysql_free_result(local_res);

        sp = tmp;

    }else if(ticketType == "直接封装+发送")
    {
        std::shared_ptr<TicketPackage> tmp = std::make_shared<TicketPackage>();

        snprintf(local_sql, SQL_MAX, "select * from package_send where work_order_id = %d;",workOrderId);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUserOrder() 查询package_send表失败！失败原因：%s", mysql_error(mysql));
            return {std::shared_ptr<Ticket>()};
        }
        local_res = mysql_store_result(mysql);
        while(local_row = mysql_fetch_row(local_res))
        {
            tmp->coordinationId = local_row[2];
            tmp->updateNote = local_row[3];
            tmp->packRequirement = local_row[4];
            tmp->interfaceChanged = atoi(local_row[5]);
            tmp->targetClient = local_row[6];
            tmp->validatedByCAE =atoi(local_row[7]);
            tmp->sensitiveInfo = local_row[8];
            tmp->remark = (local_row[13]?local_row[13]:"");

        }
        mysql_free_result(local_res);

        sp = tmp;
    }else if(ticketType == "交付发送")
    {
        std::shared_ptr<TicketDelivery> tmp = std::make_shared<TicketDelivery>();

        snprintf(local_sql, SQL_MAX, "select * from delivery_send where work_order_id = %d;",workOrderId);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUserOrder() 查询delivery_send表失败！失败原因：%s", mysql_error(mysql));
            return {std::shared_ptr<Ticket>()};
        }
        local_res = mysql_store_result(mysql);
        while(local_row = mysql_fetch_row(local_res))
        {
            tmp->targetClient = local_row[2];
            tmp->validatedByCAE = atoi(local_row[3]);
            tmp->sensitiveInfo = (local_row[4]?local_row[4]:"");

        }
        mysql_free_result(local_res);

        sp = tmp;
    }else if(ticketType == "功能开发")
    {
        std::shared_ptr<TicketFeature> tmp = std::make_shared<TicketFeature>();

        snprintf(local_sql, SQL_MAX, "select * from function_development where work_order_id = %d;",workOrderId);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUserOrder() 查询function_development表失败！失败原因：%s", mysql_error(mysql));
            return {std::shared_ptr<Ticket>()};
        }
        local_res = mysql_store_result(mysql);
        while(local_row = mysql_fetch_row(local_res))
        {
            tmp->featureInit = local_row[2];
            tmp->featureFinal = (local_row[3]?local_row[3]:"");
        }
        mysql_free_result(local_res);

        sp = tmp;
    }else if(ticketType == "其他")
    {
        std::shared_ptr<TicketOther> tmp = std::make_shared<TicketOther>();

        snprintf(local_sql, SQL_MAX, "select * from other_work_order where work_order_id = %d;",workOrderId);
        local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getUserOrder() 查询other_work_order表失败！失败原因：%s", mysql_error(mysql));
            return {std::shared_ptr<Ticket>()};
        }
        local_res = mysql_store_result(mysql);
        while(local_row = mysql_fetch_row(local_res))
        {
            tmp->description = local_row[2];
            
        }
        mysql_free_result(local_res);

        sp = tmp;
    }
    return sp;
}

bool UserDAO::downloadAttachment(std::shared_ptr<TicketReproduce> tmp)
{
    //保存文件
    IniReader config;
    MYSQL* mysql;
    DBConnectionManager::getConnection(mysql);
    char sql[SQL_MAX];	

    if (!config.load("config.ini")) {
            printf("无法读取 config.ini 文件\n");
        return false;
    }
    
    //查找附件文件的相对路径
    snprintf(sql, SQL_MAX, "select file_path,file_name from issue_reproduction_attachment where ticket_id = %d;",tmp->ticketId);
    int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        LOG_ERROR("function:downloadAttachment 查询issue_reproduction_attachment表失败！失败原因：%s", mysql_error(mysql));
        return false;
    }
    MYSQL_RES* res = mysql_store_result(mysql);
    MYSQL_ROW row;
    std::string relativePath;
    row = mysql_fetch_row(res);

    if(row)
    {
        relativePath = (row[0]?row[0]:"");
        tmp->attachment.fileName = (row[1]?row[1]:"");
    }

    mysql_free_result(res);

    // 检查是否有有效的附件信息
    if (relativePath.empty() || tmp->attachment.fileName.empty()) {
        // 设置空的附件信息
        tmp->attachment.fileName = "";
        tmp->attachment.file = "";
        DBConnectionManager::closeConnection(mysql);
        return true; // 返回true，因为没有附件不算错误
    }

    std::string filePath = config.getString("storage","upload_dir") + relativePath;

    std::ifstream ifs(filePath,std::ios::binary | std::ios::in);
    if(!ifs.is_open())
    {
        printf("无法打开附件文件 %s\n",filePath.c_str());
        return false;
    }
    ifs.seekg(0,std::ios::end);
    std::streampos size = ifs.tellg();
    ifs.seekg(0,std::ios::beg);
    std::string content(size,'\0');
    ifs.read(&content[0],size);
    ifs.close();
    tmp->attachment.file = std::move(content);

    DBConnectionManager::closeConnection(mysql);
    return true;
}

std::string UserDAO::queryModelVersion(int modelVersionId)
{
    MYSQL* mysql;
    char local_sql[SQL_MAX];	
    DBConnectionManager::getConnection(mysql);
    std::string retStr;

    snprintf(local_sql, SQL_MAX, "select version from model_version where id = %d;",modelVersionId);
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:queryModelVersion() 查询model_version表失败！失败原因：%s", mysql_error(mysql));
        return "error";
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        retStr = (local_row[0]?local_row[0]:"");
    }
    mysql_free_result(local_res);

    DBConnectionManager::closeConnection(mysql);

    return retStr;
}

std::string UserDAO::queryBaseModelVersion(int modelVersionId)
{
    MYSQL* mysql;
    char local_sql[SQL_MAX];	
    DBConnectionManager::getConnection(mysql);
    std::string retStr;
    std::string model;

    //查询模型名称
    snprintf(local_sql, SQL_MAX, "select model from model_version where id = %d;",modelVersionId);
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:queryBaseModelVersion() 查询model_version表失败！失败原因：%s", mysql_error(mysql));
        return "error";
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    if(local_row = mysql_fetch_row(local_res))
    {
        model = local_row[0];
    }
    mysql_free_result(local_res);

    //查询模型版本的前一个版本
    snprintf(local_sql, SQL_MAX, "select version from model_version where model = '%s' and id < %d order by id desc limit 1;",model.c_str(),modelVersionId);
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:queryBaseModelVersion() 查询model_version表失败！失败原因：%s", mysql_error(mysql));
        return "error";
    }
    local_res = mysql_store_result(mysql);
    if(local_row = mysql_fetch_row(local_res))
    {
        retStr = local_row[0];
    }
    mysql_free_result(local_res);

    DBConnectionManager::closeConnection(mysql);

    return retStr;
}


TicketExecutor UserDAO::queryTicketExecutor(int workOrderId)
{
    MYSQL* mysql;
    char local_sql[SQL_MAX];	
    DBConnectionManager::getConnection(mysql);
    TicketExecutor executor;

    snprintf(local_sql, SQL_MAX, "select * from work_order_executor where work_order_id = %d order by id desc;",workOrderId);
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:queryTicketExecutor() 查询work_order_executor表失败！失败原因：%s", mysql_error(mysql));
        executor.id = -1;
        return executor;
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    extern std::map<int,std::string> id_name;
    while(local_row = mysql_fetch_row(local_res))
    {
        if(local_row[2])
            executor.executor.push_back(id_name[atoi(local_row[2])]);
        else
            executor.executor.push_back("");
        if(local_row[4])
            executor.reason.push_back(local_row[4]);
        else
            executor.reason.push_back("");
        if(local_row[3])
            executor.timestamp.push_back(local_row[3]);
        else
            executor.timestamp.push_back("");

    }
    mysql_free_result(local_res);

    DBConnectionManager::closeConnection(mysql);
    return executor;
}

std::string UserDAO::queryProductAuthorization(int productAuthorizationId)
{
    MYSQL* mysql;
    char local_sql[SQL_MAX];	
    DBConnectionManager::getConnection(mysql);
    std::string retStr;

    snprintf(local_sql, SQL_MAX, "select authorization_code from product_authorization where id = %d;",productAuthorizationId);
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:queryProductAuthorization() 查询authorization_code表失败！失败原因：%s", mysql_error(mysql));
        return "error";
    }
    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        retStr = (local_row[0]?local_row[0]:"");
    }
    mysql_free_result(local_res);

    DBConnectionManager::closeConnection(mysql);

    return retStr;
}

bool UserDAO::login(std::string account, std::string password)
{
    bool retLogin = false;

    //检查数据库连接状态
    if(!ensureConnection())
    {
        LOG_ERROR("function:login() 数据库连接失败");
        return retLogin;
    }
    MYSQL* mysql = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;

    snprintf(local_sql, SQL_MAX, "select password from user where username = %d;",stoi(account));
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:login() 查询user表失败！失败原因：%s", mysql_error(mysql));
        return retLogin;
    }
    local_res = mysql_store_result(mysql);
    if(local_row = mysql_fetch_row(local_res))
    {
       if(password == std::string(local_row[0]))
            retLogin = true;
    }
    mysql_free_result(local_res);

    return retLogin;
}

void add_idname_mapping()
{
    MYSQL* mysql;
    char local_sql[SQL_MAX];	
    DBConnectionManager::getConnection(mysql);

    // 设置连接字符集为 utf8mb4
    mysql_set_character_set(mysql, "utf8mb4");

    snprintf(local_sql, SQL_MAX, "set names utf8mb4;");
    mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));

    snprintf(local_sql, SQL_MAX, "select username,real_name from user;");
    int local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));

    MYSQL_RES* local_res = mysql_store_result(mysql);
    MYSQL_ROW local_row;
    extern std::map<int,std::string> id_name;
    while(local_row = mysql_fetch_row(local_res))
    {
        // 检查 real_name 是否为空，如果为空则使用空字符串
        std::string realName = (local_row[1] != nullptr) ? local_row[1] : "";
        id_name[atoi(local_row[0])] = realName;
    }

    
    id_name[0] = "";
    mysql_free_result(local_res);

    DBConnectionManager::closeConnection(mysql);

}