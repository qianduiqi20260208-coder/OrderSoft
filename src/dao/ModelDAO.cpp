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

bool ModelDAO::addModelVersion(const std::string &model, const std::string &modelVersion)
{
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    //数据库存储
    snprintf(sql, SQL_MAX, "INSERT INTO model_version(model,version) "
        "VALUES('%s', '%s');", model.c_str(),modelVersion.c_str());	
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:addModelVersion 添加模型版本失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

std::vector<std::vector<std::string>> ModelDAO::getModelVersionInfoByModelPaged(const std::string &model, int offset, int count)
{
    std::vector<std::vector<std::string>> retVec;
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        //返回的数组中有一个空的元素代表查询失败
        retVec.push_back({});
        return retVec;
    }

    //分页查询所有模型版本
    snprintf(sql, SQL_MAX, "select version,id,update_time from model_version where model ='%s' order by id desc limit %d,%d;",model.c_str(),offset,count);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getModelVersionWithOrdersByModelPaged 查询model_version表失败！失败原因：%s\n", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::vector<std::string> tmp;
        tmp.push_back(std::string(row[0]));
        tmp.push_back(std::string(row[1]));
        tmp.push_back(std::string(row[2]));

        retVec.push_back(tmp);
    }
    mysql_free_result(res);
    return retVec;
}

//-1表示查询失败
int ModelDAO::getModelVersionCount(std::string model)
{
    int count = -1;
    //检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return count;
    }

    //分页查询所有模型版本
    snprintf(sql, SQL_MAX, "select count(*) from model_version where model ='%s';",model.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getModelVersionCount 查询model_version表失败！失败原因：%s\n", mysql_error(mysql));
        return count;
    }
    res = mysql_store_result(mysql);
    if(row = mysql_fetch_row(res))
    {
        count = atoi(row[0]);
    }
    mysql_free_result(res);

    return count;
}
