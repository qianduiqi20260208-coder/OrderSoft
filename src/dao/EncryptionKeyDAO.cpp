#include "EncryptionKeyDAO.h"
#include "DBConnectionManager.h"

EncryptionKey::EncryptionKey(MYSQL *m):mysql(m)
{
    DBConnectionManager::getConnection(mysql);
}

//返回空代表查询失败
std::vector<DongleInfo> EncryptionKey::getDongleInfo()
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {};
    }

    std::vector<DongleInfo> retVec;

    snprintf(sql, SQL_MAX, "select * from encryption_key;");
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getDongleInfo 查询 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {

        DongleInfo di;
        di.dongleId = row[0];
        di.shellCode = row[1];
        di.shellSerial = row[2];
        
        //根据外壳号去加密狗历史信息表里查询与加密狗关联的诸多信息
        snprintf(sql, SQL_MAX, "select * from encryption_key_history where encryption_key = '%s' order by id desc;",row[1]);//只查询一条数据
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:getDongleInfo 查询 encryption_key_history 表失败！失败原因：%s\n", mysql_error(mysql));
            return {};
        }
        MYSQL_RES* res = mysql_store_result(mysql);
        if(MYSQL_ROW row = mysql_fetch_row(res))
        {
            di.inTime = (row[2]?row[2]:"");
            di.outTime = (row[3]?row[3]:"");
            di.dongleStatus = row[4];
            di.clientName = row[5];
            di.clientDeviceType = row[6];
            di.clientNote = (row[7]?row[7]:"");
            di.dongleRemark = (row[8]?row[8]:"");
        }
        mysql_free_result(res);
        retVec.push_back(di);
    }
    mysql_free_result(res);

    return retVec; 
}

bool EncryptionKey::createEncryptionKey(std::string s1, std::string s2)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    snprintf(sql, SQL_MAX, "insert into encryption_key(shell_number,shell_serial_number) values('%s','%s'); ",s1.c_str(),s2.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createEncryptionKey 插入 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

bool EncryptionKey::updateEncryptionKey(int id, std::string s1, std::string s2)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    snprintf(sql, SQL_MAX, "update encryption_key set shell_number = '%s',shell_serial_number = '%s' where id = %d; ",s1.c_str(),s2.c_str(),id);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateEncryptionKey 修改 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

std::vector<std::vector<std::string>> EncryptionKey::selectAllEncryptionHistoryByEK(std::string ek)
{
    std::vector<std::vector<std::string>> retVec;
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return {};
    }

    snprintf(sql, SQL_MAX, "select out_storage_time,in_storage_time,customer from encryption_key_history where encryption_key = '%s';",ek.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAllEncryptionHistoryByEK 查询 encryption_key_history 表失败！失败原因：%s\n", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::string in_storage_time = (row[1]?row[1]:"");
        retVec.push_back({row[0],in_storage_time,row[2]});
    }
    mysql_free_result(res);

    return retVec;

}

std::vector<std::vector<std::string>> EncryptionKey::selectAllAuthInfoByClientEK(std::string client,std::string ek)
{
    std::vector<std::vector<std::string>> retVec;


    snprintf(sql, SQL_MAX,
        "SELECT "
        "    pai.created_at, "
        "    pa.authorization_code, "
        "    pai.authorization_start_date, "
        "    pai.authorization_end_date, "
        "    pai.encryption_type, "
        "    pai.remark "
        "FROM product_authorization_info AS pai "
        "JOIN product_authorization AS pa "
        "    ON pai.authorization_id = pa.id "
        "JOIN encryption_key_history AS ekh "
        "    ON ekh.customer = pa.client "
        "WHERE pa.client = '%s' "
        "  AND pa.encryption_key = '%s' "
        "  AND pa.generate_time between ekh.out_storage_time and ekh.in_storage_time; ",
        client.c_str(), ek.c_str()
    );


    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:selectAllAuthInfoByClientEK 查询 product_authorization_info 表失败！失败原因：%s\n", mysql_error(mysql));
        return {};
    }
    res = mysql_store_result(mysql);
    while(row = mysql_fetch_row(res))
    {
        std::string remark = (row[5]?row[5]:"");
        //日期时间中去掉时间
        std::string created_at = std::string(row[0]).substr(0,10);
        retVec.push_back({created_at,row[1],row[2],row[3],row[4],remark});
    }
    mysql_free_result(res);

    return retVec;
}

EncryptionKey::~EncryptionKey()
{
    DBConnectionManager::closeConnection(mysql);
}
