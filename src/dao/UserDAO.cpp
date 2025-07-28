#include "UserDAO.h"
#include "DBConnectionManager.h"
#include <sstream>
#include <fstream>
#include "IniReader.h"

UserDAO::UserDAO(MYSQL *ms):mysql(ms)
{
    DBConnectionManager::getConnection(mysql);
}

std::vector<User> UserDAO::getUser()
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {};
    }

    std::vector<User> userVec;

    //获取所有用户
    snprintf(sql, SQL_MAX, "select * from user;");
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
		printf("[error] function:getUser() 查询user表失败！失败原因：%s\n", mysql_error(mysql));
		return {};
	}
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        User user;
        user.jobNumber = atoi(row[1]);
        user.fullName = (row[3] == nullptr ? "":row[3]);
        user.canApprove = atoi(row[5]);
        user.lastLoginTime = (row[10] == nullptr ? "":row[10]);
        user.isOnline = atoi(row[11]);
        if(row[12]!=nullptr)
            user.lastUsedModel = atoi(row[12]);
        //查询用户负责的模型
        snprintf(sql, SQL_MAX, "select model from user_model where username=%d;",user.jobNumber);
        int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:getUser() 查询user_model表失败！失败原因：%s\n", mysql_error(mysql));
            return {};
        }
        MYSQL_RES* res = mysql_store_result(mysql);
        MYSQL_ROW row;
        std::vector<std::string> userModelVec;
        while(row = mysql_fetch_row(res))
        {
            userModelVec.push_back(row[0]);
        }
        user.responsibleModel = userModelVec;
        mysql_free_result(res);

        // 查出用户角色
        snprintf(sql, SQL_MAX, "select role from user_multi_role where user_id=%d;",user.jobNumber);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:getUser() 查询user_multi_role表失败！失败原因：%s\n", mysql_error(mysql));
            return {};
        }
        res = mysql_store_result(mysql);
        std::vector<std::string> userRoleVec;
        while(row = mysql_fetch_row(res))
        {
            userRoleVec.push_back(std::string(row[0]));
        }
        user.roleVec = userRoleVec;
        mysql_free_result(res);

        userVec.push_back(user);
    }

    // 释放结果集
	mysql_free_result(res);


    return userVec;
}


std::vector<std::shared_ptr<Ticket>> UserDAO::getUserOrder(int jobNumber)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {std::shared_ptr<Ticket>()};
    }

    std::vector<std::shared_ptr<Ticket>> userTicketVec;
    std::vector<int> ticketID;

    //查询用户所需要处理的全部工单ID
    snprintf(sql, SQL_MAX, "select work_order_id from user_multi_role where user_id = %d and role = 'null';",jobNumber);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getUserOrder() 查询user_multi_role表失败！失败原因：%s\n", mysql_error(mysql));
        return {std::shared_ptr<Ticket>()};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        ticketID.push_back(atoi(row[0]));
    }
    mysql_free_result(res);

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
    ss<<");";
    snprintf(sql, SQL_MAX, ss.str().c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    printf("getUserOrder() sql:%s\n",sql);
    if (ret) {
        printf("[error] function:getUserOrder() 查询work_order表失败！失败原因：%s\n", mysql_error(mysql));
        return {std::shared_ptr<Ticket>()};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::shared_ptr<Ticket> sp;

        //查询各自的表
        if(std::string(row[3]) == "问题复现")
        {
            std::shared_ptr<TicketReproduce> tmp = std::make_shared<TicketReproduce>();

            snprintf(sql, SQL_MAX, "select * from issue_reproduction where work_order_id = %d;",atoi(row[0]));
            int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
            if (ret) {
                printf("[error] function:getUserOrder() 查询issue_reproduction表失败！失败原因：%s\n", mysql_error(mysql));
                return {std::shared_ptr<Ticket>()};
            }
            //保存ticketId的值，用来下载附件
            tmp->ticketId = atoi(row[0]);

            MYSQL_RES* res = mysql_store_result(mysql);
            MYSQL_ROW row;
            while(row = mysql_fetch_row(res))
            {
                tmp->coordinationId = row[2];
                tmp->content = row[3];
            }
            mysql_free_result(res);
            //保存需要下载的附件本体 另外加上对于下载失败的处理
            if(!downloadAttachment(tmp))
            {
                return {std::shared_ptr<Ticket>()};
            }else{
                sp = tmp;
            }

        }else if(std::string(row[3]) == "版本迭代")
        {
            std::shared_ptr<TicketVersion> tmp = std::make_shared<TicketVersion>();

            snprintf(sql, SQL_MAX, "select * from version_iteration where work_order_id = %d;",atoi(row[0]));
            int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
            if (ret) {
                printf("[error] function:getUserOrder() 查询version_iteration表失败！失败原因：%s\n", mysql_error(mysql));
                return {std::shared_ptr<Ticket>()};
            }
            MYSQL_RES* res = mysql_store_result(mysql);
            MYSQL_ROW row;
            while(row = mysql_fetch_row(res))
            {
                tmp->coordinationId = row[2];
                tmp->updateNote = row[3];
                tmp->packRequirement = row[4];
                tmp->interfaceChanged = atoi(row[5]);
            }
            mysql_free_result(res);

            sp = tmp;

        }else if(std::string(row[3]) == "直接封装+发送")
        {
            std::shared_ptr<TicketPackage> tmp = std::make_shared<TicketPackage>();

            snprintf(sql, SQL_MAX, "select * from package_send where work_order_id = %d;",atoi(row[0]));
            int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
            if (ret) {
                printf("[error] function:getUserOrder() 查询package_send表失败！失败原因：%s\n", mysql_error(mysql));
                return {std::shared_ptr<Ticket>()};
            }
            MYSQL_RES* res = mysql_store_result(mysql);
            MYSQL_ROW row;
            while(row = mysql_fetch_row(res))
            {
                tmp->coordinationId = row[2];
                tmp->updateNote = row[3];
                tmp->packRequirement = row[4];
                tmp->interfaceChanged = atoi(row[5]);
                tmp->targetClient = atoi(row[6]);
                tmp->validatedByCAE =atoi(row[7]);
                tmp->sensitiveInfo = row[8];

            }
            mysql_free_result(res);

            sp = tmp;
        }else if(std::string(row[3]) == "交付发送")
        {
            std::shared_ptr<TicketDelivery> tmp = std::make_shared<TicketDelivery>();

            snprintf(sql, SQL_MAX, "select * from delivery_send where work_order_id = %d;",atoi(row[0]));
            int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
            if (ret) {
                printf("[error] function:getUserOrder() 查询delivery_send表失败！失败原因：%s\n", mysql_error(mysql));
                return {std::shared_ptr<Ticket>()};
            }
            MYSQL_RES* res = mysql_store_result(mysql);
            MYSQL_ROW row;
            while(row = mysql_fetch_row(res))
            {
                tmp->targetClient = atoi(row[2]);
                tmp->validatedByCAE = atoi(row[3]);
                tmp->sensitiveInfo = row[4];

            }
            mysql_free_result(res);

            sp = tmp;
        }else if(std::string(row[3]) == "功能开发")
        {
            std::shared_ptr<TicketFeature> tmp = std::make_shared<TicketFeature>();

            snprintf(sql, SQL_MAX, "select * from function_development where work_order_id = %d;",atoi(row[0]));
            int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
            if (ret) {
                printf("[error] function:getUserOrder() 查询function_development表失败！失败原因：%s\n", mysql_error(mysql));
                return {std::shared_ptr<Ticket>()};
            }
            MYSQL_RES* res = mysql_store_result(mysql);
            MYSQL_ROW row;
            while(row = mysql_fetch_row(res))
            {
                tmp->featureInit = row[2];
                
            }
            mysql_free_result(res);

            sp = tmp;
        }else if(std::string(row[3]) == "其他")
        {
            std::shared_ptr<TicketOther> tmp = std::make_shared<TicketOther>();

            snprintf(sql, SQL_MAX, "select * from other_work_order where work_order_id = %d;",atoi(row[0]));
            int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
            if (ret) {
                printf("[error] function:getUserOrder() 查询other_work_order表失败！失败原因：%s\n", mysql_error(mysql));
                return {std::shared_ptr<Ticket>()};
            }
            MYSQL_RES* res = mysql_store_result(mysql);
            MYSQL_ROW row;
            while(row = mysql_fetch_row(res))
            {
                tmp->description = row[2];
                
            }
            mysql_free_result(res);

            sp = tmp;
        }

        //通用的插入代码
        {
            sp->id = atoi(row[0]);
            sp->creatorId = atoi(row[1]);
            sp->createTime = row[2];
            sp->ticketType = row[3];
            sp->model = row[4];
            sp->modelVersion = row[5];
            sp->status = row[6];
            sp->approverId = atoi(row[7]);
            sp->priorityHint = (row[8] == nullptr?"":row[8]);
            sp->distributorId = atoi(row[9]);
            sp->approvedTime = (row[10] == nullptr?"":row[10]);
            sp->priorityTask =(row[11] == nullptr?"":row[11]);
            sp->distributedTime = (row[12] == nullptr?"":row[12]);
        }


        userTicketVec.push_back(sp);
    }
    mysql_free_result(res); 
    return userTicketVec;
}

UserDAO::~UserDAO()
{
    DBConnectionManager::closeConnection(mysql);
}

bool UserDAO::downloadAttachment(std::shared_ptr<TicketReproduce> tmp)
{
    //保存文件
    IniReader config;
    if (!config.load("../../config/config.ini")) { 
        printf("无法读取 config.ini 文件\n");
        return false;
    }

    //查找附件文件的相对路径
    snprintf(sql, SQL_MAX, "select file_path,file_name from issue_reproduction_attachment where ticket_id = %d;",tmp->ticketId);
    int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:downloadAttachment 查询issue_reproduction_attachment表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }
    MYSQL_RES* res = mysql_store_result(mysql);
    MYSQL_ROW row;
    std::string relativePath;
    row = mysql_fetch_row(res);
    relativePath = row[0];
    tmp->attachment.fileName = row[1];

    mysql_free_result(res);

    std::string filePath = config.getString("storage","upload_dir") + relativePath;

    std::ifstream ifs(filePath,std::ios::binary | std::ios::in);
    if(!ifs.is_open())
    {
        printf("无法打开附件文件\n");
        return false;
    }
    ifs.seekg(0,std::ios::end);
    std::streampos size = ifs.tellg();
    ifs.seekg(0,std::ios::beg);
    std::string content(size,'\0');
    ifs.read(&content[0],size);
    ifs.close();
    tmp->attachment.file = std::move(content);
    return true;
}
