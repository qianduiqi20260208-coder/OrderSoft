#include "CustomerInfoDAO.h"
#include "DBConnectionManager.h"

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

    //筛选出有效授权的数量
    snprintf(sql, SQL_MAX, "select count(*) from product_authorization_info where authorization_id in(select id from product_authorization where encryption_key = '%s') and CURDATE() BETWEEN authorization_start_date AND authorization_end_date; ",encryptionKey.c_str());
    printf("sql:%s\n",sql);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAuthorizationCountByEncryptionKey 查询 product_authorization_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return {-1};
    }
    res = mysql_store_result(mysql);
    if(row = mysql_fetch_row(res))
    {
        retVec.push_back(atoi(row[0]));
    }
    mysql_free_result(res);

    //筛选出临期授权的数量
    snprintf(sql, SQL_MAX, "select count(*) from product_authorization_info where authorization_id in(select id from product_authorization where encryption_key = '%s') and DATEDIFF(authorization_end_date, CURDATE()) BETWEEN 0 AND 5; ",encryptionKey.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAuthorizationCountByEncryptionKey 查询 product_authorization_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return {-1};
    }
    res = mysql_store_result(mysql);
    if(row = mysql_fetch_row(res))
    {
        retVec.push_back(atoi(row[0]));
    }
    mysql_free_result(res);

    //筛选出过期授权的数量
    snprintf(sql, SQL_MAX, "select count(*) from product_authorization_info where authorization_id in(select id from product_authorization where encryption_key = '%s') and authorization_end_date < CURDATE();  ",encryptionKey.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAuthorizationCountByEncryptionKey 查询 product_authorization_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return {-1};
    }
    res = mysql_store_result(mysql);
    if(row = mysql_fetch_row(res))
    {
        retVec.push_back(atoi(row[0]));
    }
    mysql_free_result(res);

    return retVec;
}

std::vector<std::vector<std::string>> CustomerInfoDAO::selectAllSendRecordByClient(std::string client)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        //返回的数组中有一个空的元素代表查询失败
        return {};
    }

    std::vector<std::vector<std::string>> retVec;

    snprintf(sql, SQL_MAX,
        "SELECT wo.model, mv.version, wo.id, wo.completed_at "
        "FROM work_order AS wo "
        "INNER JOIN model_version AS mv ON wo.model_version_id = mv.id "
        "LEFT JOIN delivery_send AS ds ON ds.work_order_id = wo.id "
        "LEFT JOIN package_send AS ps ON ps.work_order_id = wo.id "
        "WHERE (ds.target_customer = '%s' OR ps.target_customer = '%s') "
        "AND wo.completed_at IS NOT NULL "
        "ORDER BY wo.completed_at DESC;",
        client.c_str(), client.c_str()
    );

    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAllSendRecordByClient 失败原因：%s\n", mysql_error(mysql));
        return {{}};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        retVec.push_back({row[0],row[1],row[2],row[3]});
    }
    mysql_free_result(res);

    return retVec;
}

std::vector<std::vector<std::string>> CustomerInfoDAO::selectLatestModelVersionByClient(std::string client)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        //返回的数组中有一个空的元素代表查询失败
        return {};
    }

    std::vector<std::vector<std::string>> retVec;

    snprintf(sql, SQL_MAX,
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


    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectLatestModelVersionByClient 失败原因：%s\n", mysql_error(mysql));
        return {{}};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        retVec.push_back({row[0],row[1],row[2],row[3]});
    }
    mysql_free_result(res);

    return retVec;
}

CustomerInfoDAO::~CustomerInfoDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
