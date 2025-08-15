#include "EncryptionKeyDAO.h"
#include "DBConnectionManager.h"
#include <ctime>
#include <iomanip>
#include <sstream>

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

    snprintf(sql, SQL_MAX_, "select * from encryption_key;");
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
        snprintf(sql, SQL_MAX_, "select * from encryption_key_history where encryption_key = '%s' order by id desc;",row[1]);//只查询一条数据
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

    snprintf(sql, SQL_MAX_, "insert into encryption_key(shell_number,shell_serial_number) values('%s','%s'); ",s1.c_str(),s2.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createEncryptionKey 插入 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

// 获取可交付的外壳号列表
std::vector<std::pair<int, std::string>> EncryptionKey::getAvailableShellNumbers()
{
    std::vector<std::pair<int, std::string>> availableShells;

    // 检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return availableShells;
    }

    // 优化后的SQL查询：获取可交付的外壳号
    // 1. 从未出库的外壳号（在encryption_key_history中没有记录）
    // 2. 最新状态为"入库"的外壳号
    snprintf(sql, SQL_MAX_,
             "SELECT DISTINCT ek.id, ek.shell_number "
             "FROM encryption_key ek "
             "WHERE ek.shell_number NOT IN ("
                 "SELECT DISTINCT ekh1.encryption_key "
                 "FROM encryption_key_history ekh1 "
                 "WHERE ekh1.encryption_key = ek.shell_number "
                 "AND ekh1.id = ("
                     "SELECT MAX(ekh2.id) "
                     "FROM encryption_key_history ekh2 "
                     "WHERE ekh2.encryption_key = ekh1.encryption_key"
                 ") "
                 "AND (ekh1.status = '出库' OR ekh1.status = '损坏' OR ekh1.status = '丢失')"
             ") "
             "ORDER BY ek.id");

    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:getAvailableShellNumbers 查询可交付外壳号失败！失败原因：%s\n", mysql_error(mysql));
        return availableShells;
    }

    res = mysql_store_result(mysql);
    if (!res) {
        printf("[error] function:getAvailableShellNumbers 获取查询结果失败！\n");
        return availableShells;
    }

    // 处理查询结果
    while((row = mysql_fetch_row(res))) {
        if (row[0] && row[1]) {
            int id = std::stoi(row[0]);
            std::string shellNumber = row[1];
            availableShells.push_back(std::make_pair(id, shellNumber));
        }
    }

    mysql_free_result(res);

    return availableShells;
}

bool EncryptionKey::updateEncryptionKey(int id, std::string s1, std::string s2)
{
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    snprintf(sql, SQL_MAX_, "update encryption_key set shell_number = '%s',shell_serial_number = '%s' where id = %d; ",s1.c_str(),s2.c_str(),id);
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateEncryptionKey 修改 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    return true;
}

bool EncryptionKey::deliveryOperation(const std::string& clientName,
                                     const std::string& shellNumber,
                                     const std::string& deviceType,
                                     const std::string& deviceNote)
{
    // 检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    // 开始事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:deliveryOperation 开始事务失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    // 1. 检查encryption_key表中是否已存在该外壳号
    snprintf(sql, SQL_MAX_, "SELECT id FROM encryption_key WHERE shell_number = '%s'", shellNumber.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:deliveryOperation 查询 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    bool shellExists = (mysql_num_rows(res) > 0);
    mysql_free_result(res);

    // 检查外壳号是否存在，如果不存在则不允许交付
    if (!shellExists) {
        printf("[error] function:deliveryOperation 外壳号 '%s' 在 encryption_key 表中不存在，无法进行交付操作！\n", shellNumber.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 2. 获取当前时间作为出库时间
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");
    std::string outStorageTime = ss.str();

    // 5. 检查设备类型是否有效
    if (deviceType != "lab" && deviceType != "IPT" && deviceType != "FTD" && deviceType != "FFS") {
        printf("[error] function:deliveryOperation 设备类型 '%s' 无效！有效值为: lab, IPT, FTD, FFS\n", deviceType.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    // 3. 检查客户名称是否存在于customer_info表中
    snprintf(sql, SQL_MAX_, "SELECT id FROM customer_info WHERE customer_name = '%s'", clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:deliveryOperation 查询 customer_info 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    bool customerExists = (mysql_num_rows(res) > 0);
    mysql_free_result(res);

    if (!customerExists) {
        printf("[error] function:deliveryOperation 客户名称 '%s' 不存在于 customer_info 表中！\n", clientName.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 4. 检查外壳号的当前状态，确保可以进行交付操作
    // 检查外壳号在记录表中最新一次操作是否为入库
    snprintf(sql, SQL_MAX_, "SELECT status FROM encryption_key_history WHERE encryption_key = '%s' ORDER BY id DESC LIMIT 1", shellNumber.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:deliveryOperation 查询外壳号最新状态失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    bool canDeliver = true;
    std::string statusMessage = "";

    if (mysql_num_rows(res) == 0) {
        // 没有历史记录，说明是全新的外壳号，可以交付
        canDeliver = true;
        statusMessage = "全新外壳号，可以交付";
    } else {
        // 有历史记录，检查最新状态是否为入库
        MYSQL_ROW row = mysql_fetch_row(res);
        std::string lastStatus = row[0] ? row[0] : "";

        if (lastStatus == "入库") {
            canDeliver = true;
            statusMessage = "外壳号最新状态为入库，可以交付";
        } else {
            canDeliver = false;
            statusMessage = "外壳号最新状态不是入库，无法交付";
        }
    }
    mysql_free_result(res);

    if (!canDeliver) {
        printf("[error] function:deliveryOperation 外壳号 '%s' %s\n", shellNumber.c_str(), statusMessage.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    printf("[info] function:deliveryOperation 外壳号 '%s' %s\n", shellNumber.c_str(), statusMessage.c_str());


    // 6. 向encryption_key_history表插入数据，status为"出库"，out_storage_time有值，in_storage_time为NULL
    snprintf(sql, SQL_MAX_,
             "INSERT INTO encryption_key_history(encryption_key, in_storage_time, out_storage_time, status, customer, customer_device_type, customer_pc_remark, created_at) "
             "VALUES('%s', NULL, '%s', '出库', '%s', '%s', '%s', NOW())",
             shellNumber.c_str(), outStorageTime.c_str(), clientName.c_str(), deviceType.c_str(), deviceNote.c_str());

    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:deliveryOperation 插入 encryption_key_history 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 提交事务
    if (mysql_real_query(mysql, "COMMIT", strlen("COMMIT"))) {
        printf("[error] function:deliveryOperation 提交事务失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    return true;
}

// 入库操作：更新encryption_key_history表中的入库时间和状态
bool EncryptionKey::returnOperation(const std::string& clientName,
                                   const std::string& shellNumber,
                                   const std::string& operationType,
                                   const std::string& inTime,
                                   const std::string& outTime,
                                   const std::string& remark)
{
    // 检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    // 开始事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:returnOperation 开始事务失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    // 1. 检查客户名称是否存在于customer_info表中
    snprintf(sql, SQL_MAX_, "SELECT id FROM customer_info WHERE customer_name = '%s'", clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:returnOperation 查询 customer_info 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    bool customerExists = (mysql_num_rows(res) > 0);
    mysql_free_result(res);

    if (!customerExists) {
        printf("[error] function:returnOperation 客户名称 '%s' 不存在于 customer_info 表中！\n", clientName.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 2. 检查encryption_key表中是否存在该外壳号
    snprintf(sql, SQL_MAX_, "SELECT id FROM encryption_key WHERE shell_number = '%s'", shellNumber.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:returnOperation 查询 encryption_key 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    bool shellExists = (mysql_num_rows(res) > 0);
    mysql_free_result(res);

    if (!shellExists) {
        printf("[error] function:returnOperation 外壳号 '%s' 不存在于 encryption_key 表中！\n", shellNumber.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 3. 检查是否存在未归还的出库记录并获取设备信息（status='出库'且in_storage_time为NULL）
    snprintf(sql, SQL_MAX_,
             "SELECT id, customer_device_type, customer_pc_remark FROM encryption_key_history WHERE encryption_key = '%s' AND customer = '%s' AND status = '出库' ORDER BY created_at DESC LIMIT 1",
             shellNumber.c_str(), clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:returnOperation 查询 encryption_key_history 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    if (mysql_num_rows(res) == 0) {
        printf("[error] function:returnOperation 未找到对应的出库记录！客户：%s，外壳号：%s\n", clientName.c_str(), shellNumber.c_str());
        mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 获取出库记录的设备信息，用于入库记录
    row = mysql_fetch_row(res);
    std::string deviceType = row[1] ? row[1] : "lab";  // 默认为lab
    std::string deviceRemark = row[2] ? row[2] : "";
    mysql_free_result(res);

    // 4. 插入新的入库记录到encryption_key_history表
    snprintf(sql, SQL_MAX_,
             "INSERT INTO encryption_key_history(encryption_key, in_storage_time, out_storage_time, status, customer, customer_device_type, customer_pc_remark, created_at) "
             "VALUES('%s','%s','%s', '%s', '%s', '%s', '%s', NOW())",
             shellNumber.c_str(), inTime.c_str(), outTime.c_str(), operationType.c_str(), clientName.c_str(), deviceType.c_str(), deviceRemark.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:returnOperation 插入 encryption_key_history 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    // 5. 设置授权为归还
    snprintf(sql, SQL_MAX_, "UPDATE `model_life_manager`.`product_authorization` SET `return` = '1' WHERE `encryption_key` = '%s'", shellNumber.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:returnOperation 更新 product_authorization 表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    // 6.TODO设置授权的结束时间

    // 7. 如果操作类型为'损坏'或'丢失'，更新encryption_key表的remark字段
    if (operationType == "损坏" || operationType == "丢失") {
        snprintf(sql, SQL_MAX_, "UPDATE encryption_key SET remark = '%s' WHERE shell_number = '%s'", remark.c_str(), shellNumber.c_str());
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:returnOperation 更新 encryption_key 表 remark 字段失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        printf("[info] function:returnOperation 外壳号 %s 因 %s 更新备注：%s\n", shellNumber.c_str(), operationType.c_str(), remark.c_str());
    }

    // 提交事务
    if (mysql_real_query(mysql, "COMMIT", strlen("COMMIT"))) {
        printf("[error] function:returnOperation 提交事务失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    return true;
}

bool EncryptionKey::createAuthorization(const std::string& clientName,
                                       const std::string& shellNumber,
                                       const std::string& authId,
                                       const std::string& authType,
                                       const std::string& startDate,
                                       const std::string& endDate,
                                       const std::string& authNote)
{
    // 检查数据库连接状态
    if(!DBConnectionManager::ensureConnected(mysql))
    {
        return false;
    }

    // 开始事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:createAuthorization 开始事务失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    // 1. 验证客户是否存在
    snprintf(sql, SQL_MAX_, "SELECT customer_name FROM customer_info WHERE customer_name = '%s'", clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createAuthorization 查询客户信息失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if (!res || mysql_num_rows(res) == 0) {
        printf("[error] function:createAuthorization 客户不存在：%s\n", clientName.c_str());
        if (res) mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    mysql_free_result(res);

    // 2. 验证外壳号是否存在且当前分配给指定客户
    snprintf(sql, SQL_MAX_,
        "SELECT ek.shell_number "
        "FROM encryption_key ek "
        "JOIN encryption_key_history ekh ON ek.shell_number = ekh.encryption_key "
        "JOIN ("
        "    SELECT encryption_key, MAX(id) as max_id "
        "    FROM encryption_key_history "
        "    GROUP BY encryption_key"
        ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
        "WHERE ek.shell_number = '%s' AND ekh.customer = '%s' AND ekh.status = '出库'",
        shellNumber.c_str(), clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createAuthorization 查询外壳号分配状态失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    if (!res || mysql_num_rows(res) == 0) {
        printf("[error] function:createAuthorization 外壳号不存在或未分配给客户 %s：%s\n", clientName.c_str(), shellNumber.c_str());
        if (res) mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    mysql_free_result(res);

    // 3. 获取或创建product_authorization记录
    unsigned long long authorizationId = 0;

    // 首先查询是否已存在该外壳号和授权代码的记录
    snprintf(sql, SQL_MAX_, "SELECT id FROM product_authorization WHERE encryption_key = '%s' AND authorization_code = '%s'",
             shellNumber.c_str(), authId.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createAuthorization 查询授权记录失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    if (res && mysql_num_rows(res) > 0) {
        // 如果记录已存在，获取其ID
        MYSQL_ROW row = mysql_fetch_row(res);
        authorizationId = strtoull(row[0], NULL, 10);
        mysql_free_result(res);

        // 更新product_authorization表的客户信息
        snprintf(sql, SQL_MAX_, "UPDATE product_authorization SET customer_name = '%s' WHERE id = %llu",
                 clientName.c_str(), authorizationId);
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:createAuthorization 更新授权表客户信息失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }

        // 更新product_authorization_info表
        snprintf(sql, SQL_MAX_,
                 "UPDATE product_authorization_info SET encryption_type = '%s', authorization_start_date = '%s', "
                 "authorization_end_date = '%s', remark = '%s' WHERE authorization_id = %llu",
                 authType.c_str(), startDate.c_str(), endDate.c_str(), authNote.c_str(), authorizationId);
    } else {
        // 如果记录不存在，创建新记录
        if (res) mysql_free_result(res);

        // 插入product_authorization表
        snprintf(sql, SQL_MAX_, "INSERT INTO product_authorization (encryption_key, authorization_code, customer_name) VALUES ('%s', '%s', '%s')",
                 shellNumber.c_str(), authId.c_str(), clientName.c_str());
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:createAuthorization 插入授权表失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }

        // 获取插入的授权ID
        authorizationId = mysql_insert_id(mysql);

        // 插入product_authorization_info表
        snprintf(sql, SQL_MAX_,
                 "INSERT INTO product_authorization_info (authorization_id, encryption_type, authorization_start_date, authorization_end_date, remark) "
                 "VALUES (%llu, '%s', '%s', '%s', '%s')",
                 authorizationId, authType.c_str(), startDate.c_str(), endDate.c_str(), authNote.c_str());
    }

    // 4. 执行SQL语句（更新或插入product_authorization_info）
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:createAuthorization 更新或插入授权信息表失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 提交事务
    if (mysql_real_query(mysql, "COMMIT", strlen("COMMIT"))) {
        printf("[error] function:createAuthorization 提交事务失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    printf("[info] function:createAuthorization 创建授权成功！外壳号：%s，授权ID：%s\n", shellNumber.c_str(), authId.c_str());
    return true;
}

bool EncryptionKey::updateAuthorizationEndDates(const std::string& clientName,
                                               const std::vector<std::tuple<std::string, std::string, std::string>>& changes)
{
    char sql[SQL_MAX_];
    int ret;

    if (!mysql) {
        printf("[error] function:updateAuthorizationEndDates MySQL连接为空！\n");
        return false;
    }

    // 开始事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:updateAuthorizationEndDates 开始事务失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    // 1. 验证客户是否存在
    snprintf(sql, SQL_MAX_, "SELECT customer_name FROM customer_info WHERE customer_name = '%s'", clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateAuthorizationEndDates 查询客户信息失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if (!res || mysql_num_rows(res) == 0) {
        printf("[error] function:updateAuthorizationEndDates 客户不存在：%s\n", clientName.c_str());
        if (res) mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    mysql_free_result(res);

    // 2. 逐个处理更新请求
    for (const auto& change : changes) {
        const std::string& shellNumber = std::get<0>(change);
        const std::string& authId = std::get<1>(change);
        const std::string& newEndDate = std::get<2>(change);

        // 验证授权记录是否存在且属于指定客户的外壳号
        snprintf(sql, SQL_MAX_,
            "SELECT pai.id "
            "FROM product_authorization pa "
            "JOIN product_authorization_info pai ON pa.id = pai.authorization_id "
            "JOIN encryption_key_history ekh ON pa.encryption_key = ekh.encryption_key "
            "JOIN ("
            "    SELECT encryption_key, MAX(id) as max_id "
            "    FROM encryption_key_history "
            "    GROUP BY encryption_key"
            ") latest ON ekh.encryption_key = latest.encryption_key AND ekh.id = latest.max_id "
            "WHERE pa.encryption_key = '%s' AND pa.authorization_code = '%s' "
            "AND ekh.customer = '%s' AND ekh.status = '出库'",
            shellNumber.c_str(), authId.c_str(), clientName.c_str());
        
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:updateAuthorizationEndDates 查询授权记录失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }

        res = mysql_store_result(mysql);
        if (!res || mysql_num_rows(res) == 0) {
            printf("[error] function:updateAuthorizationEndDates 授权记录不存在或不属于客户 %s：外壳号=%s, 授权ID=%s\n", 
                   clientName.c_str(), shellNumber.c_str(), authId.c_str());
            if (res) mysql_free_result(res);
            mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }
        
        MYSQL_ROW row = mysql_fetch_row(res);
        std::string authInfoId = row[0];
        mysql_free_result(res);

        // 更新授权截止日期
        snprintf(sql, SQL_MAX_,
            "UPDATE product_authorization_info "
            "SET authorization_end_date = '%s' "
            "WHERE id = %s",
            newEndDate.c_str(), authInfoId.c_str());
        
        ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
        if (ret) {
            printf("[error] function:updateAuthorizationEndDates 更新授权截止日期失败！失败原因：%s\n", mysql_error(mysql));
            mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
            return false;
        }

        printf("[info] function:updateAuthorizationEndDates 成功更新授权：外壳号=%s, 授权ID=%s, 新截止日期=%s\n",
               shellNumber.c_str(), authId.c_str(), newEndDate.c_str());
    }

    // 提交事务
    if (mysql_real_query(mysql, "COMMIT", strlen("COMMIT"))) {
        printf("[error] function:updateAuthorizationEndDates 提交事务失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    printf("[info] function:updateAuthorizationEndDates 批量更新授权截止日期成功，共更新 %zu 条记录\n", changes.size());
    return true;
}

bool EncryptionKey::updateShellDeviceInfo(const std::string& clientName,
                                         const std::string& shellNumber,
                                         const std::string& deviceType,
                                         const std::string& deviceNote)
{
    char sql[SQL_MAX_];
    int ret;

    if (!mysql) {
        printf("[error] function:updateShellDeviceInfo MySQL连接为空！\n");
        return false;
    }

    // 开始事务
    if (mysql_real_query(mysql, "START TRANSACTION", strlen("START TRANSACTION"))) {
        printf("[error] function:updateShellDeviceInfo 开始事务失败！失败原因：%s\n", mysql_error(mysql));
        return false;
    }

    // 1. 验证客户是否存在
    snprintf(sql, SQL_MAX_, "SELECT customer_name FROM customer_info WHERE customer_name = '%s'", clientName.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateShellDeviceInfo 查询客户信息失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    MYSQL_RES* res = mysql_store_result(mysql);
    if (!res || mysql_num_rows(res) == 0) {
        printf("[error] function:updateShellDeviceInfo 客户不存在：%s\n", clientName.c_str());
        if (res) mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    mysql_free_result(res);

    // 2. 验证外壳号是否存在
    snprintf(sql, SQL_MAX_, "SELECT shell_number FROM encryption_key WHERE shell_number = '%s'", shellNumber.c_str());
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateShellDeviceInfo 查询外壳号失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    if (!res || mysql_num_rows(res) == 0) {
        printf("[error] function:updateShellDeviceInfo 外壳号不存在：%s\n", shellNumber.c_str());
        if (res) mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    mysql_free_result(res);

    // 3. 验证设备类型是否有效
    if (deviceType != "lab" && deviceType != "IPT" && deviceType != "FTD" && deviceType != "FFS") {
        printf("[error] function:updateShellDeviceInfo 设备类型 '%s' 无效！有效值为: lab, IPT, FTD, FFS\n", deviceType.c_str());
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 4. 查找指定客户当前持有的外壳号的最新出库记录
    snprintf(sql, SQL_MAX_,
        "SELECT id FROM encryption_key_history "
        "WHERE encryption_key = '%s' AND customer = '%s' AND status = '出库' "
        "ORDER BY id DESC LIMIT 1",
        shellNumber.c_str(), clientName.c_str());
    
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateShellDeviceInfo 查询外壳号出库记录失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    res = mysql_store_result(mysql);
    if (!res || mysql_num_rows(res) == 0) {
        printf("[error] function:updateShellDeviceInfo 外壳号 %s 未分配给客户 %s 或已归还\n", 
               shellNumber.c_str(), clientName.c_str());
        if (res) mysql_free_result(res);
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    std::string historyId = row[0];
    mysql_free_result(res);

    // 5. 更新外壳号的设备信息
    snprintf(sql, SQL_MAX_,
        "UPDATE encryption_key_history "
        "SET customer_device_type = '%s', customer_pc_remark = '%s' "
        "WHERE id = %s",
        deviceType.c_str(), deviceNote.c_str(), historyId.c_str());
    
    ret = mysql_real_query(mysql, sql, (unsigned long)strlen(sql));
    if (ret) {
        printf("[error] function:updateShellDeviceInfo 更新设备信息失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    // 提交事务
    if (mysql_real_query(mysql, "COMMIT", strlen("COMMIT"))) {
        printf("[error] function:updateShellDeviceInfo 提交事务失败！失败原因：%s\n", mysql_error(mysql));
        mysql_real_query(mysql, "ROLLBACK", strlen("ROLLBACK"));
        return false;
    }

    printf("[info] function:updateShellDeviceInfo 成功更新外壳号设备信息：客户=%s, 外壳号=%s, 设备类型=%s\n",
           clientName.c_str(), shellNumber.c_str(), deviceType.c_str());
    return true;
}

EncryptionKey::~EncryptionKey()
{
    DBConnectionManager::closeConnection(mysql);
}
