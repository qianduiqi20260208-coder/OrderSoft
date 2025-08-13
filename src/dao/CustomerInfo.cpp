#include "CustomerInfoDAO.h"
#include "DBConnectionManager.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

CustomerInfoDAO::CustomerInfoDAO(MYSQL *ms) : mysql(ms)
{
    DBConnectionManager::getConnection(mysql);
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
            "    ci.customer_name = '%s' AND ek.shell_number = '%s' AND ekh.status = '出库' ;",
            // "    AND (pa.generate_time IS NULL OR pa.generate_time >= ekh.out_storage_time)", 
            clientName.c_str(), shellNumber.c_str());
        
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:getClientAuthInfo() 查询授权信息失败！失败原因：%s\n", mysql_error(mysql));
            continue;
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
                
                // 添加授权信息到对应的ShellNumber
                shellNumberMap[shellNumber].authorizationList.push_back(auth);
                shellNumberMap[shellNumber].authCount++;
            }
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

CustomerInfoDAO::~CustomerInfoDAO()
{
    DBConnectionManager::closeConnection(mysql);
}