#include "ModelDAO.h"
#include "DBConnectionManager.h"
#include "Logger.h"

ModelDAO::ModelDAO() : mysql(nullptr)
{
    // 使用连接池，不需要初始化mysql指针
}

ModelDAO::ModelDAO(MYSQL *ms):mysql(ms)
{
    LOG_WARNING("ModelDAO: 使用已废弃的构造函数，建议使用连接池");
    DBConnectionManager::getConnection(mysql);
}

std::vector<std::string> ModelDAO::getModel()
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        LOG_ERROR("function:getModel 获取数据库连接失败");
        return {};
    }
    
    MYSQL* conn = getConnection();
    std::vector<std::string> modelVec;

    snprintf(sql, SQL_MAX, "select ata_code from model order by id asc;");
    
    ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
    if (ret) {
        LOG_ERROR("function:getModel 查询model表失败！失败原因：%s", mysql_error(conn));
        return {};
    }
    res = mysql_store_result(conn);
    while(row = mysql_fetch_row(res))
    {
        modelVec.push_back(std::string(row[0]));
    }
    mysql_free_result(res);

    return modelVec;
}

std::vector<std::string> ModelDAO::getModelVersionByModel(std::string modelName)
{
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return {};
    }
    
    MYSQL* conn = getConnection();
    std::vector<std::string> modelVersion;

    snprintf(sql, SQL_MAX, "select version from model_version  where model ='%s' order by id desc;",modelName.c_str());
    ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
    if (ret) {
        LOG_ERROR("function:getModelVersionByModel 查询model_version表失败！失败原因：%s", mysql_error(conn));
        return {};
    }
    res = mysql_store_result(conn);
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
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return false;
    }
    
    MYSQL* conn = getConnection();
    //数据库存储
    snprintf(sql, SQL_MAX, "INSERT INTO model_version(model,version) "
        "VALUES('%s', '%s');", model.c_str(),modelVersion.c_str());	
    ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:addModelVersion 添加模型版本失败！失败原因：%s", mysql_error(conn));
            return false;
        }

    return true;
}

std::vector<std::vector<std::string>> ModelDAO::getModelVersionInfoByModelPaged(const std::string &model, int offset, int count)
{
    std::vector<std::vector<std::string>> retVec;
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        //返回的数组中有一个空的元素代表查询失败
        retVec.push_back({});
        return retVec;
    }
    
    MYSQL* conn = getConnection();
    //分页查询所有模型版本
    snprintf(sql, SQL_MAX, "select version,id,update_time from model_version where model ='%s' order by id desc limit %d,%d;",model.c_str(),offset,count);
    ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:getModelVersionInfoByModelPaged 查询model_version表失败！失败原因：%s", mysql_error(conn));
            return {};
        }
    res = mysql_store_result(conn);
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
    // 使用BaseDAO的优化连接管理
    if (!ensureConnection()) {
        return count;
    }
    
    MYSQL* conn = getConnection();
    //分页查询所有模型版本
    snprintf(sql, SQL_MAX, "select count(*) from model_version where model ='%s';",model.c_str());
    ret = mysql_real_query(conn, sql, (unsigned long)strlen(sql));
        if (ret) {
            LOG_ERROR("function:getModelVersionCount 查询model_version表失败！失败原因：%s", mysql_error(conn));
            return count;
        }
    res = mysql_store_result(conn);
    if(row = mysql_fetch_row(res))
    {
        count = atoi(row[0]);
    }
    mysql_free_result(res);

    return count;
}

std::vector<std::pair<std::string, std::string>> ModelDAO::selectModelUpdateNotesByModelName(std::string modelName)
{
    if(!ensureConnection())
    {
        return {};
    }
    MYSQL* mysql = getConnection();
    std::vector<std::pair<std::string, std::string>> retVec;

    snprintf(sql, SQL_MAX, "select mv.version,vi.update_content from version_iteration vi"
        " join model_version mv on vi.new_model_version_id = mv.id"
        " join work_order wo on wo.id = vi.work_order_id where wo.model ='%s';",modelName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        LOG_ERROR("function:selectModelUpdateNotesByModelName 查询model_version表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        retVec.push_back({row[0],(row[1]?row[1]:"")});
    }
    mysql_free_result(res);

    //新旧updateNotes的区分
    retVec.push_back({"----","--------------以下为历史更新记录--------------"});

    snprintf(sql, SQL_MAX, "select model_version,update_content from history_updatenotes where model_name = '%s';",modelName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        LOG_ERROR("function:selectModelUpdateNotesByModelName 查询model_version表失败！失败原因：%s", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        retVec.push_back({row[0],(row[1]?row[1]:"")});
    }
    mysql_free_result(res);

    return retVec;
}
