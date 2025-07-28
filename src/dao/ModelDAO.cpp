#include "ModelDAO.h"
#include "DBConnectionManager.h"

ModelDAO::ModelDAO(MYSQL *ms):mysql(ms)
{
    DBConnectionManager::getConnection(mysql);
}

std::vector<std::string> ModelDAO::getModel()
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {};
    }

    std::vector<std::string> modelVec;

    snprintf(sql, SQL_MAX, "select ata_code from model order by id asc;");
    
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getModel 查询model表失败！失败原因：%s\n", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        modelVec.push_back(std::string(row[0]));
    }
    mysql_free_result(res);

    return modelVec;
}

std::vector<std::string> ModelDAO::getModelVersionByModel(std::string modelName)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {};
    }

    std::vector<std::string> modelVersion;

    snprintf(sql, SQL_MAX, "select version from model_version where model ='%s' order by id desc;",modelName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getModelVersionByModel 查询model_version表失败！失败原因：%s\n", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        modelVersion.push_back(std::string(row[0]));
    }
    mysql_free_result(res);
    
    return modelVersion;
}

ModelDAO::~ModelDAO()
{
    DBConnectionManager::closeConnection(mysql);
}
