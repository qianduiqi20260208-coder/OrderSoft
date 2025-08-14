#include "CustomerInfoDAO.h"
#include "DBConnectionManager.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

CustomerInfoDAO::CustomerInfoDAO(MYSQL *m):mysql(m)
{
    DBConnectionManager::getConnection(mysql);
}



bool CustomerInfoDAO::createClient(std::string s1, std::string s2)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    snprintf(sql, SQL_MAX, "insert into customer_info(customer_name,remarks) values('%s','%s'); ",s1.c_str(),s2.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createClient 插入 customer_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

bool CustomerInfoDAO::updateClient(std::string originClient, std::string newClient, std::string clientRemark)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    snprintf(sql, SQL_MAX, "update customer_info set customer_name = '%s',remarks = '%s' where customer_name = '%s'; ",originClient.c_str(),newClient.c_str(),clientRemark.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateClient 修改 customer_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

std::vector<std::pair<std::string, std::string>> CustomerInfoDAO::selectAllClientInfo()
{
    std::vector<std::pair<std::string, std::string>> retVec;

    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        //返回的数组中有一个空的pair代表查询失败
        return {std::pair<std::string, std::string>()};
    }

    //查询出库时间以及入库时间
    snprintf(sql, SQL_MAX, "select customer_name,remarks from customer_info;");
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAllClientInfo 查询 customer_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return {std::pair<std::string, std::string>()};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::pair<std::string, std::string> tmp;
        tmp.first = row[0];
        tmp.second = (row[1]?row[1]:"");
        retVec.push_back(tmp);
    }
    mysql_free_result(res);

    return retVec;
}

std::vector<std::string> CustomerInfoDAO::selectEncryptionKeyByClient(std::string client)
{
    std::vector<std::string> retVec;
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        //返回的数组中有一个空的元素代表查询失败
        return {""};
    }

    //查询出库时间以及入库时间
    snprintf(sql, SQL_MAX, "select distinct encryption_key from encryption_key_history where customer ='%s' and status = '出库';",client.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectEncryptionKeyByClient 查询 encryption_key_history 表失败！失败原因：%s\n", mysql_error(mysql));
        return {""};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        retVec.push_back(row[0]);
    }
    mysql_free_result(res);

    return retVec;
}

std::pair<int, int> CustomerInfoDAO::selectModelAndModelVersionCountByClient(std::string client)
{
    std::pair<int, int> pr;
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        //返回的数组中有一个空的元素代表查询失败
        return {-1,-1};
    }

    //查询模型的数量（去重）
    snprintf(sql, SQL_MAX, "select count(distinct model) from work_order where id in(select work_order_id from delivery_send where target_customer = '%s' union select work_order_id from package_send where target_customer = '%s'); ",client.c_str(),client.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectModelAndModelVersionCountByClient 查询 work_order 表失败！失败原因：%s\n", mysql_error(mysql));
        return {-1,-1};
    }
    res = mysql_store_result(mysql);
    if(row = mysql_fetch_row(res))
    {
        pr.first = atoi(row[0]);
    }
    mysql_free_result(res);

    //查询模型版本的数量（不用去重）
    snprintf(sql, SQL_MAX, "select count(model_version_id) from work_order where id in(select work_order_id from delivery_send where target_customer = '%s' union select work_order_id from package_send where target_customer = '%s'); ",client.c_str(),client.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectModelAndModelVersionCountByClient 查询 work_order 表失败！失败原因：%s\n", mysql_error(mysql));
        return {-1,-1};
    }
    res = mysql_store_result(mysql);
    if(row = mysql_fetch_row(res))
    {
        pr.second = atoi(row[0]);
    }
    mysql_free_result(res);

    return pr;
}

std::vector<int> CustomerInfoDAO::selectAuthorizationCountByEncryptionKey(std::string encryptionKey)
{
    std::vector<int> retVec;


    return retVec;
}
Client CustomerInfoDAO::getClientAuthInfo(const std::string& clientName)
{
    Client client;
    
    // 检查数据库连接状态
    if (!DBConnectionManager::ensureConnected(mysql))
    {
        return client;
    }
    
    // 设置客户名称
    client.clientName = clientName;
    
    // 查询该客户当前状态为"出库"的加密狗信息
    snprintf(sql, 1024, 
        "SELECT "
        "    ek.shell_number, "
        "    ekh.customer_device_type, "
        "    ekh.customer_pc_remark, "
        "    ekh.in_storage_time, "
        "    ekh.out_storage_time, "
        "    ekh.status "
        "FROM "
        "    customer_info ci "
        "JOIN "
        "    encryption_key_history ekh ON ci.customer_name = ekh.customer "
        "JOIN "
        "    encryption_key ek ON ekh.encryption_key = ek.shell_number "
        "JOIN ("
        "    SELECT encryption_key, MAX(id) as max_id "
        "    FROM encryption_key_history "
        "    GROUP BY encryption_key"
        ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
        "WHERE "
        "    ci.customer_name = '%s' AND ekh.status = '出库';", 
        clientName.c_str());
    
    int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getClientAuthInfo() 查询加密狗信息失败！失败原因：%s\n", mysql_error(mysql));
        return client;
    }
    
    MYSQL_RES* res = mysql_store_result(mysql);
    MYSQL_ROW row;
    
    // 用于存储所有的shell_number，避免重复
    std::map<std::string, ShellNumber> shellNumberMap;
    
    // 处理查询结果，构建ShellNumber列表
    while (row = mysql_fetch_row(res))
    {
        std::string shellNumber = row[0] ? row[0] : "";
        
        // 如果这个shell_number还没有处理过，则添加到map中
        if (shellNumberMap.find(shellNumber) == shellNumberMap.end())
        {
            ShellNumber shellInfo;
            shellInfo.shellNumber = shellNumber;
            shellInfo.deviceType = row[1] ? row[1] : "";
            shellInfo.deviceNote = row[2] ? row[2] : "";
            shellInfo.outTime = row[4] ? row[4] : "";


            shellInfo.authCount = 0; // 初始化授权数量为0
            
            shellNumberMap[shellNumber] = shellInfo;
        }
    }
    mysql_free_result(res);
    
    // 对每个shell_number查询其授权信息
    for (auto& pair : shellNumberMap)
    {
        std::string shellNumber = pair.first;
        
        // 查询该shell_number当前状态为"出库"的授权信息
        // 关键修改：只查询当前出库周期内生成的授权，避免查询到之前归还前的授权
        snprintf(sql, 1024, 
            "SELECT "
            "    pa.authorization_code, "
            "    pai.authorization_start_date, "
            "    pai.authorization_end_date, "
            "    pai.encryption_type, "
            "    pai.remark, "
            "    ekh.status "
            "FROM "
            "    customer_info ci "
            "JOIN "
            "    encryption_key_history ekh ON ci.customer_name = ekh.customer "
            "JOIN "
            "    encryption_key ek ON ekh.encryption_key = ek.shell_number "
            "JOIN ("
            "    SELECT encryption_key, MAX(id) as max_id "
            "    FROM encryption_key_history "
            "    GROUP BY encryption_key"
            ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
            "LEFT JOIN "
            "    product_authorization pa ON ek.shell_number = pa.encryption_key "
            "LEFT JOIN "
            "    product_authorization_info pai ON pa.id = pai.authorization_id "
            "WHERE "
            "    ci.customer_name = '%s' AND ek.shell_number = '%s' AND ekh.status = '出库' AND pa.return = '0' ;",

            // "    AND (pa.generate_time IS NULL OR pa.generate_time >= ekh.out_storage_time)", 
            clientName.c_str(), shellNumber.c_str());
        
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:getClientAuthInfo() 查询授权信息失败！失败原因：%s\n", mysql_error(mysql));
            continue;
        }
        
        res = mysql_store_result(mysql);
        bool haveAuth = false;
        Authorization lastAuth;
        // 处理查询结果，构建Authorization列表
        while (row = mysql_fetch_row(res))
        {
            // 只有当授权码不为空时才添加授权信息
            if (row[0] && strlen(row[0]) > 0)
            {
                Authorization auth;
                auth.authId = row[0] ? row[0] : "";
                auth.startDate = row[1] ? row[1] : "";
                auth.endDate = row[2] ? row[2] : "";
                auth.authType = row[3] ? row[3] : "";
                auth.authNote = row[4] ? row[4] : "";
                
                shellNumberMap[shellNumber].authCount++;

                // 添加授权信息到对应的ShellNumber 只添加最后一个
                haveAuth = true;
                lastAuth = auth;
                //shellNumberMap[shellNumber].authorizationList.push_back(auth);
            }
        }
        if (haveAuth)
        {
            shellNumberMap[shellNumber].authorizationList.push_back(lastAuth);
        }
        
        mysql_free_result(res);
    }
    
    // 将map中的ShellNumber转换为vector
    for (const auto& pair : shellNumberMap)
    {
        client.shellNumbers.push_back(pair.second);
    }
    
    // 设置dongleCount
    client.dongleCount = client.shellNumbers.size();
    
    return client;
}

std::vector<std::string> CustomerInfoDAO::getAllClientNames()
{
    std::vector<std::string> clientNames;
    
    // 检查数据库连接状态
    if (!DBConnectionManager::ensureConnected(mysql))
    {
        return clientNames;
    }
    
    // 查询所有客户名称
    snprintf(sql, 1024, "SELECT customer_name FROM customer_info;");
    
    int ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getAllClientNames() 查询客户名称失败！失败原因：%s\n", mysql_error(mysql));
        return clientNames;
    }
    
    MYSQL_RES* res = mysql_store_result(mysql);
    MYSQL_ROW row;
    
    // 处理查询结果
    while (row = mysql_fetch_row(res))
    {
        if (row[0])
        {
            clientNames.push_back(row[0]);
        }
    }
    mysql_free_result(res);
    
    return clientNames;
}


std::vector<Authorization> CustomerInfoDAO::getShellAuthorizationInfo(const std::string& clientName, const std::string& shellNumber)
{
    std::vector<Authorization> authorizationList;
    
    // 查询该shell_number当前状态为"出库"的授权信息
    // 关键修改：只查询当前出库周期内生成的授权，避免查询到之前归还前的授权
    snprintf(sql, 1024, 
        "SELECT "
        "    pa.authorization_code, "
        "    pai.authorization_start_date, "
        "    pai.authorization_end_date, "
        "    pai.encryption_type, "
        "    pai.remark, "
        "    ekh.status "
        "FROM "
        "    customer_info ci "
        "JOIN "
        "    encryption_key_history ekh ON ci.customer_name = ekh.customer "
        "JOIN "
        "    encryption_key ek ON ekh.encryption_key = ek.shell_number "
        "JOIN ("
        "    SELECT encryption_key, MAX(id) as max_id "
        "    FROM encryption_key_history "
        "    GROUP BY encryption_key"
        ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
        "LEFT JOIN "
        "    product_authorization pa ON ek.shell_number = pa.encryption_key "
        "LEFT JOIN "
        "    product_authorization_info pai ON pa.id = pai.authorization_id "
        "WHERE "
        "    ci.customer_name = '%s' AND ek.shell_number = '%s' AND ekh.status = '出库' AND pa.return = '0' ;",
        // "    AND (pa.generate_time IS NULL OR pa.generate_time >= ekh.out_storage_time)", 
        clientName.c_str(), shellNumber.c_str());
    
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getShellAuthorizationInfo() 查询授权信息失败！失败原因：%s\n", mysql_error(mysql));
        return authorizationList;
    }
    
    res = mysql_store_result(mysql);
    
    // 处理查询结果，构建Authorization列表
    while (row = mysql_fetch_row(res))
    {
        // 只有当授权码不为空时才添加授权信息
        if (row[0] && strlen(row[0]) > 0)
        {
            Authorization auth;
            auth.authId = row[0] ? row[0] : "";
            auth.startDate = row[1] ? row[1] : "";
            auth.endDate = row[2] ? row[2] : "";
            auth.authType = row[3] ? row[3] : "";
            auth.authNote = row[4] ? row[4] : "";
            
            authorizationList.push_back(auth);
        }
    }
    
    mysql_free_result(res);
    return authorizationList;
}

CustomerInfoDAO::~CustomerInfoDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
