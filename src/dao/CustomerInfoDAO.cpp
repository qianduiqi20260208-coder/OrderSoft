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


    return retVec;
}


CustomerInfoDAO::~CustomerInfoDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
