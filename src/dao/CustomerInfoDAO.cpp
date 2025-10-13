#include "CustomerInfoDAO.h"
#include "DBConnectionManager.h"
#include "Logger.h"

CustomerInfoDAO::CustomerInfoDAO() : mysql(nullptr)
{
    // 使用连接池，不需要初始化MYSQL对象
}

CustomerInfoDAO::CustomerInfoDAO(MYSQL *m):mysql(m)
{
    LOG_WARNING("CustomerInfoDAO: 使用已废弃的构造函数，建议使用连接池");
    DBConnectionManager::getConnection(mysql);
}



bool CustomerInfoDAO::createClient(std::string s1, std::string s2)
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:createClient 获取数据库连接失败");
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    snprintf(local_sql, SQL_MAX, "insert into customer_info(customer_name,remarks) values('%s','%s'); ",s1.c_str(),s2.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:createClient 插入 customer_info 表失败！失败原因：%s", mysql_error(conn));
        return false;
    }

    return true;
}

bool CustomerInfoDAO::updateClient(std::string originClient, std::string newClient, std::string clientRemark)
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:updateClient 获取数据库连接失败");
        return false;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;

    snprintf(local_sql, SQL_MAX, "update customer_info set customer_name = '%s',remarks = '%s' where customer_name = '%s'; ",  newClient.c_str(), clientRemark.c_str(), originClient.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:updateClient 修改 customer_info 表失败！失败原因：%s", mysql_error(conn));
        return false;
    }

    return true;
}

std::vector<std::pair<std::string, std::string>> CustomerInfoDAO::selectAllClientInfo()
{
    std::vector<std::pair<std::string, std::string>> retVec;

    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:selectAllClientInfo 获取数据库连接失败");
        return {std::pair<std::string, std::string>()};
    }
    
    MYSQL* conn = getConnection();
    //查询出库时间以及入库时间
    char local_sql[SQL_MAX];
    int local_ret;
    
    snprintf(local_sql, SQL_MAX, "select customer_name,remarks from customer_info;");
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
     if (local_ret) {
         LOG_ERROR("function:selectAllClientInfo 查询 customer_info 表失败！失败原因：%s", mysql_error(conn));
         return {};
     }
    MYSQL_RES* local_res = mysql_store_result(conn);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        std::pair<std::string, std::string> tmp;
        tmp.first = local_row[0];
        tmp.second = (local_row[1]?local_row[1]:"");
        retVec.push_back(tmp);
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::string> CustomerInfoDAO::selectEncryptionKeyByClient(std::string client)
{
    std::vector<std::string> retVec;
    
    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        LOG_ERROR("function:selectEncryptionKeyByClient 获取数据库连接失败");
        return retVec;
    }
    
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    
    // 参考getClientAuthInfo方法的写法，查询当前状态为"出库"的加密狗
    snprintf(local_sql, SQL_MAX, 
        "SELECT DISTINCT ek.shell_number "
        "FROM customer_info ci "
        "JOIN encryption_key_history ekh ON ci.customer_name = ekh.customer "
        "JOIN encryption_key ek ON ekh.encryption_key = ek.shell_number "
        "JOIN ("
        "    SELECT encryption_key, MAX(id) as max_id "
        "    FROM encryption_key_history "
        "    GROUP BY encryption_key"
        ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
        "WHERE ci.customer_name = '%s' AND ekh.status = '出库';", 
        client.c_str());
    
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectEncryptionKeyByClient 查询失败！失败原因：%s", mysql_error(conn.get()));
        return retVec;
    }
    
    local_res = mysql_store_result(conn.get());
    while(local_row = mysql_fetch_row(local_res))
    {
        if (local_row[0]) {
            retVec.push_back(local_row[0]);
        }
    }
    mysql_free_result(local_res);

    return retVec;
}

std::pair<int, int> CustomerInfoDAO::selectModelAndModelVersionCountByClient(std::string client)
{
    std::pair<int, int> pr;
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:selectModelAndModelVersionCountByClient 获取数据库连接失败");
        return {-1,-1};
    }
    
    MYSQL* conn = getConnection();
    //查询模型的数量（去重）
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, SQL_MAX, "select count(distinct model) from work_order where id in(select work_order_id from delivery_send where target_customer = '%s' and completed_at is not null  union select work_order_id from package_send where target_customer = '%s' and completed_at is not null); ",client.c_str(),client.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectModelAndModelVersionCountByClient 查询 work_order 表失败！失败原因：%s", mysql_error(conn));
        return {-1,-1};
    }
    local_res = mysql_store_result(conn);
    if(local_row = mysql_fetch_row(local_res))
    {
        pr.first = atoi(local_row[0]);
    }
    mysql_free_result(local_res);

    //查询模型版本的数量（不用去重）
    snprintf(local_sql, SQL_MAX, "select count(model_version_id) from work_order where id in(select work_order_id from delivery_send where target_customer = '%s' and completed_at is not null union select work_order_id from package_send where target_customer = '%s' and completed_at is not null); ",client.c_str(),client.c_str());
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectModelAndModelVersionCountByClient 查询 work_order 表失败！失败原因：%s", mysql_error(conn));
        return {-1,-1};
    }
    local_res = mysql_store_result(conn);
    if(local_row = mysql_fetch_row(local_res))
    {
        pr.second = atoi(local_row[0]);
    }
    mysql_free_result(local_res);

    return pr;
}


std::vector<int> CustomerInfoDAO::selectAuthorizationCountByEncryptionKey(std::string encryptionKey)
{
    std::vector<int> retVec;

    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        LOG_ERROR("function:selectAuthorizationCountByEncryptionKey 获取数据库连接失败");
        return {-1};
    }

    //筛选出有效授权的数量
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, SQL_MAX, "select count(*) from product_authorization_info where authorization_id in(select id from product_authorization where encryption_key = '%s') and CURDATE() BETWEEN authorization_start_date AND authorization_end_date; ",encryptionKey.c_str());
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectAuthorizationCountByEncryptionKey 查询 product_authorization_info 表失败！失败原因：%s", mysql_error(conn.get()));
        return {-1};
    }
    local_res = mysql_store_result(conn.get());
    if(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    //筛选出临期授权的数量
    snprintf(local_sql, SQL_MAX, "select count(*) from product_authorization_info where authorization_id in(select id from product_authorization where encryption_key = '%s') and DATEDIFF(authorization_end_date, CURDATE()) BETWEEN 0 AND 5; ",encryptionKey.c_str());
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectAuthorizationCountByEncryptionKey 查询 product_authorization_info 表失败！失败原因：%s", mysql_error(conn.get()));
        return {-1};
    }
    local_res = mysql_store_result(conn.get());
    if(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    //筛选出过期授权的数量
    snprintf(local_sql, SQL_MAX, "select count(*) from product_authorization_info where authorization_id in(select id from product_authorization where encryption_key = '%s') and authorization_end_date < CURDATE();  ",encryptionKey.c_str());
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectAuthorizationCountByEncryptionKey 查询 product_authorization_info 表失败！失败原因：%s", mysql_error(conn.get()));
        return {-1};
    }
    local_res = mysql_store_result(conn.get());
    if(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(atoi(local_row[0]));
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::vector<std::string>> CustomerInfoDAO::selectAllSendRecordByClient(std::string client)
{
    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        LOG_ERROR("function:selectAllSendRecordByClient 获取数据库连接失败");
        return {};
    }

    std::vector<std::vector<std::string>> retVec;

    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;

    //使用联合查询
    snprintf(local_sql, SQL_MAX,
        "(SELECT wo.model, mv.version, wo.id, wo.completed_at "
        "FROM work_order AS wo "
        "INNER JOIN delivery_send AS ds ON ds.work_order_id = wo.id "
        "INNER JOIN model_version AS mv ON wo.model_version_id = mv.id "
        "WHERE ds.target_customer = '%s' AND wo.completed_at IS NOT NULL) "
        "UNION "
        "(SELECT wo.model, mv.version, wo.id, wo.completed_at "
        "FROM work_order AS wo "
        "INNER JOIN package_send AS ps ON ps.work_order_id = wo.id "
        "INNER JOIN model_version AS mv ON ps.new_model_version_id = mv.id "
        "WHERE ps.target_customer = '%s' AND wo.completed_at IS NOT NULL) "
        "ORDER BY completed_at DESC;",
        client.c_str(), client.c_str()
    );


    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectAllSendRecordByClient 失败原因：%s", mysql_error(conn.get()));
        return {{}};
    }
    local_res = mysql_store_result(conn.get());
    while(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back({local_row[0],local_row[1],local_row[2],local_row[3]});
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::vector<std::string>> CustomerInfoDAO::selectLatestModelVersionByClient(std::string client)
{
    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        LOG_ERROR("function:selectLatestModelVersionByClient 获取数据库连接失败");
        return {};
    }

    std::vector<std::vector<std::string>> retVec;

    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, SQL_MAX,
    "SELECT wo.model, wo.id, mv.version, wo.completed_at "
    "FROM work_order AS wo "
    "JOIN model_version AS mv ON wo.model_version_id = mv.id "
    "JOIN ("
    "  SELECT w2.model, MAX(w2.completed_at) AS latest_completed_at "
    "  FROM work_order AS w2 "
    "  WHERE w2.completed_at IS NOT NULL "
    "    AND ("
    "      EXISTS (SELECT 1 FROM delivery_send ds2 "
    "              WHERE ds2.work_order_id = w2.id "
    "                AND ds2.target_customer = '%s') "
    "      OR EXISTS (SELECT 1 FROM package_send ps2 "
    "              WHERE ps2.work_order_id = w2.id "
    "                AND ps2.target_customer = '%s') "
    "    ) "
    "  GROUP BY w2.model"
    ") t ON t.model = wo.model AND t.latest_completed_at = wo.completed_at "
    "WHERE wo.completed_at IS NOT NULL "
    "  AND ("
    "    EXISTS (SELECT 1 FROM delivery_send ds "
    "            WHERE ds.work_order_id = wo.id "
    "              AND ds.target_customer = '%s') "
    "    OR EXISTS (SELECT 1 FROM package_send ps "
    "            WHERE ps.work_order_id = wo.id "
    "              AND ps.target_customer = '%s') "
    "  ) "
    "ORDER BY wo.completed_at DESC;",
    client.c_str(), client.c_str(),   // 子查询里的两个 %s
    client.c_str(), client.c_str()    // 外层条件的两个 %s
    );


    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectLatestModelVersionByClient 失败原因：%s", mysql_error(conn.get()));
        return {{}};
    }
    local_res = mysql_store_result(conn.get());
    while(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back({local_row[0],local_row[1],local_row[2],local_row[3]});
    }
    mysql_free_result(local_res);

    return retVec;
}

Client CustomerInfoDAO::getClientAuthInfo(const std::string& clientName)
{
    Client client;
    
    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        return client;
    }
    
    // 设置客户名称
    client.clientName = clientName;
    
    // 查询该客户当前状态为"出库"的加密狗信息
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, 1024, 
        "SELECT "
        "    ek.shell_number, "
        "    ekh.customer_device_type, "
        "    ekh.customer_pc_remark, "
        "    ekh.in_storage_time, "
        "    ekh.out_storage_time, "
        "    ci.remarks, " // 客户信息备注
        "    ekh.contract_name, " // 合同名称
        "    ekh.contract_number, " // 合同编号
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
    
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getClientAuthInfo() 查询加密狗信息失败！失败原因：%s", mysql_error(conn.get()));
        return client;
    }
    
    local_res = mysql_store_result(conn.get());
    
    // 用于存储所有的shell_number，避免重复
    std::map<std::string, ShellNumber> shellNumberMap;
    
    // 处理查询结果，构建ShellNumber列表
    while (local_row = mysql_fetch_row(local_res))
    {
        std::string shellNumber = local_row[0] ? local_row[0] : "";
        client.remarks = local_row[5] ? local_row[5] : ""; // 客户信息备注
        // 如果这个shell_number还没有处理过，则添加到map中
        if (shellNumberMap.find(shellNumber) == shellNumberMap.end())
        {
            ShellNumber shellInfo;
            shellInfo.shellNumber = shellNumber;
            shellInfo.deviceType = local_row[1] ? local_row[1] : "";
            shellInfo.deviceNote = local_row[2] ? local_row[2] : "";
            shellInfo.contractName = local_row[6] ? local_row[6] : ""; // 合同名称
            shellInfo.contractNumber = local_row[7] ? local_row[7] : ""; // 合同编号
            shellInfo.outTime = local_row[4] ? local_row[4] : "";


            shellInfo.authCount = 0; // 初始化授权数量为0
            
            shellNumberMap[shellNumber] = shellInfo;
        }
    }
    mysql_free_result(local_res);
    
    // 对每个shell_number查询其授权信息
    for (auto& pair : shellNumberMap)
    {
        std::string shellNumber = pair.first;
        
        // 查询该shell_number当前状态为"出库"的授权信息
        // 关键修改：只查询当前出库周期内生成的授权，避免查询到之前归还前的授权
        snprintf(local_sql, 1024, 
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
        
        local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
        if (local_ret) {
            LOG_ERROR("function:getClientAuthInfo() 查询授权信息失败！失败原因：%s", mysql_error(conn.get()));
            continue;
        }
        
        local_res = mysql_store_result(conn.get());
        
        // 处理查询结果，构建Authorization列表
        while (local_row = mysql_fetch_row(local_res))
        {
            // 只有当授权码不为空时才添加授权信息
            if (local_row[0] && strlen(local_row[0]) > 0)
            {
                Authorization auth;
                auth.authId = local_row[0] ? local_row[0] : "";
                auth.startDate = local_row[1] ? local_row[1] : "";
                auth.endDate = local_row[2] ? local_row[2] : "";
                auth.authType = local_row[3] ? local_row[3] : "";
                auth.authNote = local_row[4] ? local_row[4] : "";
                
                shellNumberMap[shellNumber].authCount++;
                shellNumberMap[shellNumber].authorizationList.push_back(auth);
            }
        }
        
        mysql_free_result(local_res);
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
    
    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        LOG_ERROR("function:createClient 获取数据库连接失败");
        return clientNames;
    }
    
    // 查询所有客户名称
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, 1024, "SELECT customer_name FROM customer_info;");
    
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getAllClientNames() 查询客户名称失败！失败原因：%s", mysql_error(conn.get()));
        return clientNames;
    }
    
    local_res = mysql_store_result(conn.get());
    
    // 处理查询结果
    while (local_row = mysql_fetch_row(local_res))
    {
        if (local_row[0])
        {
            clientNames.push_back(local_row[0]);
        }
    }
    mysql_free_result(local_res);
    
    return clientNames;
}


std::vector<Authorization> CustomerInfoDAO::getShellAuthorizationInfo(const std::string& clientName, const std::string& shellNumber)
{
    std::vector<Authorization> authorizationList;
    MYSQL* mysql = getConnection();
    // 查询该shell_number当前状态为"出库"的授权信息
    // 关键修改：只查询当前出库周期内生成的授权，避免查询到之前归还前的授权
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, 1024, 
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
    
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getShellAuthorizationInfo() 查询授权信息失败！失败原因：%s", mysql_error(mysql));
        return authorizationList;
    }
    
    local_res = mysql_store_result(mysql);
    
    // 处理查询结果，构建Authorization列表
    while (local_row = mysql_fetch_row(local_res))
    {
        // 只有当授权码不为空时才添加授权信息
        if (local_row[0] && strlen(local_row[0]) > 0)
        {
            Authorization auth;
            auth.authId = local_row[0] ? local_row[0] : "";
            auth.startDate = local_row[1] ? local_row[1] : "";
            auth.endDate = local_row[2] ? local_row[2] : "";
            auth.authType = local_row[3] ? local_row[3] : "";
            auth.authNote = local_row[4] ? local_row[4] : "";
            
            authorizationList.push_back(auth);
        }
    }
    
    mysql_free_result(local_res);
    return authorizationList;
}

std::vector<std::string> CustomerInfoDAO::selectAuthorizationByEncryptionKey(std::string encryptionKey)
{
    if (!DBConnectionManager::ensureConnected(mysql))
    {
        return {};
    }
    
    std::vector<std::string> retVec;
    char local_sql[SQL_MAX];
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    snprintf(local_sql, SQL_MAX, "select authorization_code from product_authorization pa inner join product_authorization_info pai on pai.authorization_id = pa.id where pa.encryption_key = '%s' and CURDATE() BETWEEN pai.authorization_start_date AND pai.authorization_end_date; ",encryptionKey.c_str());
    local_ret = mysql_real_query(mysql, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectAuthorizationByEncryptionKey 查询 product_authorization 表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    local_res = mysql_store_result(mysql);
    if(local_row = mysql_fetch_row(local_res))
    {
        retVec.push_back(local_row[0]);
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::pair<std::string, std::string>> CustomerInfoDAO::getAllClientSuffixes()
{
    std::vector<std::pair<std::string, std::string>> retVec;
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:getAllClientSuffixes 获取数据库连接失败");
        return {};
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    snprintf(local_sql, SQL_MAX, "select customer_name, suffix from customer_info where suffix is not null and suffix != '';");
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getAllClientSuffixes 查询 customer_info 表失败！失败原因：%s", mysql_error(conn));
        return {};
    }
    MYSQL_RES* local_res = mysql_store_result(conn);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        if (local_row[0] && local_row[1]) {
            retVec.push_back(std::make_pair(local_row[0], local_row[1]));
        }
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::string> CustomerInfoDAO::selectShellsByAuthorizationCode(const std::string& authorizationCode, const std::string& clientName)
{
    std::vector<std::string> retVec;
    
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:selectShellsByAuthorizationCode 获取数据库连接失败");
        return retVec;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    // 根据授权代码和客户名称查询对应的外壳号列表
    snprintf(local_sql, SQL_MAX, 
        "SELECT DISTINCT pa.encryption_key "
        "FROM product_authorization pa "
        "WHERE pa.authorization_code = '%s' AND pa.client = '%s';",
        authorizationCode.c_str(), clientName.c_str());
        
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectShellsByAuthorizationCode 查询 product_authorization 表失败！失败原因：%s", mysql_error(conn));
        return retVec;
    }
    
    MYSQL_RES* local_res = mysql_store_result(conn);
    MYSQL_ROW local_row;
    while(local_row = mysql_fetch_row(local_res))
    {
        if (local_row[0]) {
            retVec.push_back(local_row[0]);
        }
    }
    mysql_free_result(local_res);

    return retVec;
}

std::vector<std::vector<std::string>> CustomerInfoDAO::getCustomerAuthorizationsByGroup(const std::string& clientName)
{
    std::vector<std::vector<std::string>> result;
    
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:getCustomerAuthorizationsByGroup 获取数据库连接失败");
        return result;
    }
    
    MYSQL* conn = getConnection();
    char local_sql[SQL_MAX];
    int local_ret;
    
    snprintf(local_sql, SQL_MAX, 
        "SELECT "
        "    pa.authorization_code, "
        "    pai.authorization_start_date, "
        "    pai.authorization_end_date, "
        "    pai.encryption_type, "
        "    pai.remark, "
        "    ekh.status, "
        "    ek.shell_number, "
        "    pa.id authorization_id "
        "FROM "
        "    customer_info ci "
        "JOIN "
        "    encryption_key_history ekh ON ci.customer_name = ekh.customer "
        "JOIN "
        "    encryption_key ek ON ekh.encryption_key = ek.shell_number "
        "JOIN ( "
        "    SELECT encryption_key, MAX(id) as max_id "
        "    FROM encryption_key_history "
        "    GROUP BY encryption_key "
        ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
        "LEFT JOIN "
        "    product_authorization pa ON ek.shell_number = pa.encryption_key "
        "LEFT JOIN "
        "    product_authorization_info pai ON pa.id = pai.authorization_id "
        "WHERE "
        "    ci.customer_name = '%s' "
        "    AND ekh.status = '出库' AND pa.return = '0' "
        "ORDER BY pa.authorization_code, ek.shell_number;",
        clientName.c_str());
    
    local_ret = mysql_real_query(conn, local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:getCustomerAuthorizationsByGroup 查询失败！失败原因：%s", mysql_error(conn));
        return result;
    }
    
    MYSQL_RES* local_res = mysql_store_result(conn);
    MYSQL_ROW local_row;
    
    while ((local_row = mysql_fetch_row(local_res))) {
        std::vector<std::string> row;
        for (int i = 0; i < 8; i++) {
            row.push_back(local_row[i] ? local_row[i] : "");
        }
        result.push_back(row);
    }
    
    mysql_free_result(local_res);
    return result;
}

nlohmann::json CustomerInfoDAO::selectCustomerModelVersionHistory(const std::string& clientName)
{
    nlohmann::json result = nlohmann::json::array();
    
    // 使用连接池获取连接
    ConnectionGuard conn = DBConnectionManager::getPoolConnection();
    if (!conn) {
        LOG_ERROR("function:selectCustomerModelVersionHistory 获取数据库连接失败");
        return result;
    }
    
    char local_sql[SQL_MAX * 2]; // 使用更大的缓冲区
    int local_ret;
    MYSQL_RES* local_res;
    MYSQL_ROW local_row;
    
    // 查询客户发送的所有模型版本历史，使用联合查询获取发送记录
    snprintf(local_sql, SQL_MAX * 2, 
        "SELECT wo.model, mv.version, wo.id as work_order_id, wo.completed_at, t.target_customer "
        "FROM ("
        "    SELECT wo.model, mv.version, wo.id, wo.completed_at, ds.target_customer "
        "    FROM work_order AS wo "
        "    INNER JOIN delivery_send AS ds ON ds.work_order_id = wo.id "
        "    INNER JOIN model_version AS mv ON wo.model_version_id = mv.id "
        "    WHERE wo.completed_at IS NOT NULL "
        "    UNION "
        "    SELECT wo.model, mv.version, wo.id, wo.completed_at, ps.target_customer "
        "    FROM work_order AS wo "
        "    INNER JOIN package_send AS ps ON ps.work_order_id = wo.id "
        "    INNER JOIN model_version AS mv ON ps.new_model_version_id = mv.id "
        "    WHERE wo.completed_at IS NOT NULL "
        ") t "
        "INNER JOIN work_order wo ON t.id = wo.id "
        "INNER JOIN model_version mv ON t.version = mv.version AND t.model = mv.model "
        "WHERE t.target_customer = '%s' "
        "ORDER BY t.model, wo.completed_at DESC;",
        clientName.c_str());
    
    local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
    if (local_ret) {
        LOG_ERROR("function:selectCustomerModelVersionHistory 查询失败！失败原因：%s", mysql_error(conn.get()));
        return result;
    }
    
    local_res = mysql_store_result(conn.get());
    
    std::string currentModel;
    nlohmann::json currentModelObj;
    
    while ((local_row = mysql_fetch_row(local_res))) {
        std::string modelAtaCode = local_row[0] ? local_row[0] : "";
        std::string version = local_row[1] ? local_row[1] : "";
        std::string workOrderId = local_row[2] ? local_row[2] : "";
        std::string completedDate = local_row[3] ? local_row[3] : "";
        std::string targetCustomer = local_row[4] ? local_row[4] : "";
        
        // 获取模型中文名（需要额外的查询，这里暂时使用ata_code作为名称）
        std::string modelChineseName = modelAtaCode; // 临时解决方案
        
        // 如果是新模型，创建新的模型对象
        if (currentModel != modelAtaCode) {
            if (!currentModel.empty()) {
                result.push_back(currentModelObj);
            }
            currentModel = modelAtaCode;
            currentModelObj = nlohmann::json::object();
            currentModelObj["model_ata_code"] = modelAtaCode;
            currentModelObj["model_chinese_name"] = modelChineseName;
            currentModelObj["latest_version"] = version; // 第一个记录就是最后发送的版本
            currentModelObj["versions"] = nlohmann::json::array();
        }
        
        // 添加版本信息
        nlohmann::json versionObj;
        versionObj["version"] = version;
        versionObj["work_order_id"] = workOrderId;
        versionObj["completed_date"] = completedDate;
        versionObj["is_latest"] = (currentModelObj["latest_version"] == version); // 只有最后发送的版本设为true
        
        currentModelObj["versions"].push_back(versionObj);
    }
    
    // 添加最后一个模型
    if (!currentModel.empty()) {
        result.push_back(currentModelObj);
    }
    
    mysql_free_result(local_res);
    
    // 为每个模型获取中文名
    for (auto& modelObj : result) {
        std::string ataCode = modelObj["model_ata_code"];
        snprintf(local_sql, SQL_MAX, 
            "SELECT model_name FROM model WHERE ata_code = '%s' LIMIT 1;",
            ataCode.c_str());
        
        local_ret = mysql_real_query(conn.get(), local_sql, (unsigned long)strlen(local_sql));
        if (!local_ret) {
            MYSQL_RES* name_res = mysql_store_result(conn.get());
            if (name_res) {
                MYSQL_ROW name_row = mysql_fetch_row(name_res);
                if (name_row && name_row[0]) {
                    modelObj["model_chinese_name"] = name_row[0];
                }
                mysql_free_result(name_res);
            }
        }
    }
    
    return result;
}

CustomerInfoDAO::~CustomerInfoDAO()
{
    DBConnectionManager::closeConnection(mysql);
}

bool CustomerInfoDAO::migrateCustomerName(std::string oldName, std::string newName, std::string newClientInfo)
{
    // 使用事务确保原子性
    TransactionManager tm(this);
    if (!tm.begin()) {
        LOG_ERROR("function:migrateCustomerName 开始事务失败");
        return false;
    }

    MYSQL* conn = tm.getConnection();
    int local_ret;

    auto exec = [&](const char* sql){
        local_ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (local_ret) {
            LOG_ERROR("function:migrateCustomerName 执行SQL失败: %s, 错误: %s", sql, mysql_error(conn));
            return false;
        }
        return true;
    };

    // 1. 删除外键约束（使用准确的约束名）
    if (!exec("ALTER TABLE delivery_send DROP FOREIGN KEY delivery_send_ibfk_2;")) { tm.rollback(); return false; }
    if (!exec("ALTER TABLE package_send DROP FOREIGN KEY package_send_ibfk_2;")) { tm.rollback(); return false; }
    if (!exec("ALTER TABLE encryption_key_history DROP FOREIGN KEY encryption_key_history_ibfk_2;")) { tm.rollback(); return false; }
    if (!exec("ALTER TABLE product_authorization DROP FOREIGN KEY product_authorization_ibfk_2;")) { tm.rollback(); return false; }

    // 2. 批量更新客户名称引用
    char sqlBuf[SQL_MAX];

    snprintf(sqlBuf, SQL_MAX, "UPDATE `customer_info` SET `customer_name` = '%s' WHERE `customer_name` = '%s';", newName.c_str(), oldName.c_str());
    if (!exec(sqlBuf)) { tm.rollback(); return false; }

    snprintf(sqlBuf, SQL_MAX, "UPDATE `delivery_send` SET `target_customer` = '%s' WHERE `target_customer` = '%s';", newName.c_str(), oldName.c_str());
    if (!exec(sqlBuf)) { tm.rollback(); return false; }

    snprintf(sqlBuf, SQL_MAX, "UPDATE `package_send` SET `target_customer` = '%s' WHERE `target_customer` = '%s';", newName.c_str(), oldName.c_str());
    if (!exec(sqlBuf)) { tm.rollback(); return false; }

    snprintf(sqlBuf, SQL_MAX, "UPDATE `encryption_key_history` SET `customer` = '%s' WHERE `customer` = '%s';", newName.c_str(), oldName.c_str());
    if (!exec(sqlBuf)) { tm.rollback(); return false; }

    snprintf(sqlBuf, SQL_MAX, "UPDATE `product_authorization` SET `client` = '%s' WHERE `client` = '%s';", newName.c_str(), oldName.c_str());
    if (!exec(sqlBuf)) { tm.rollback(); return false; }

    // 3. 重新添加外键约束，保持原有的 RESTRICT/CASCADE 规则
    if (!exec("ALTER TABLE delivery_send ADD CONSTRAINT delivery_send_ibfk_2 FOREIGN KEY (target_customer) REFERENCES customer_info (customer_name) ON DELETE RESTRICT ON UPDATE RESTRICT;")) { tm.rollback(); return false; }
    if (!exec("ALTER TABLE package_send ADD CONSTRAINT package_send_ibfk_2 FOREIGN KEY (target_customer) REFERENCES customer_info (customer_name) ON DELETE RESTRICT ON UPDATE RESTRICT;")) { tm.rollback(); return false; }
    if (!exec("ALTER TABLE encryption_key_history ADD CONSTRAINT encryption_key_history_ibfk_2 FOREIGN KEY (customer) REFERENCES customer_info (customer_name) ON DELETE CASCADE ON UPDATE CASCADE;")) { tm.rollback(); return false; }
    if (!exec("ALTER TABLE product_authorization ADD CONSTRAINT product_authorization_ibfk_2 FOREIGN KEY (client) REFERENCES customer_info (customer_name) ON DELETE RESTRICT ON UPDATE RESTRICT;")) { tm.rollback(); return false; }
    // 4. 更新客户remarks(clientinfo)
    snprintf(sqlBuf, SQL_MAX, "UPDATE `customer_info` SET `remarks` = '%s' WHERE `customer_name` = '%s';", newClientInfo.c_str(), newName.c_str());
    if (!exec(sqlBuf)) { tm.rollback(); return false; }    
    if (!tm.commit()) {
        LOG_ERROR("function:migrateCustomerName 提交事务失败");
        tm.rollback();
        return false;
    }

    return true;
}
