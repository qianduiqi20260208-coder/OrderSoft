#pragma once
#include "ICustomerInfoDAO.h"
#include "BaseDAO.h"
#include <mysql.h>
#define SQL_MAX 99999		// sql语句字符数组最大值

class CustomerInfoDAO : public ICustomerInfoDAO, public BaseDAO
{
public:
    CustomerInfoDAO(); // 使用连接池，不需要MYSQL参数
    CustomerInfoDAO(MYSQL* ms); // 兼容旧接口（已废弃）

    bool createClient(std::string,std::string) override;
    //第一个string是原客户名称，第二个string是新客户名称，第三个string是客户信息备注
    bool updateClient(std::string,std::string,std::string) override;

    std::vector<std::pair<std::string,std::string>> selectAllClientInfo() override;
    std::vector<std::string> selectEncryptionKeyByClient(std::string) override;
    std::pair<int,int> selectModelAndModelVersionCountByClient(std::string) override;

    //第一个元素是有效授权数量 第二个元素是临期授权数量 第三个元素是过期授权数量
    std::vector<int> selectAuthorizationCountByEncryptionKey(std::string) override;

    //根据客户筛选出所有的发送记录 vector里依次是模型名称、模型版本号、工单号、发送时间
    std::vector<std::vector<std::string>> selectAllSendRecordByClient(std::string) override;

    std::vector<std::vector<std::string>> selectLatestModelVersionByClient(std::string client) override;
    // 根据客户名称获取客户信息和加密狗授权信息
    Client getClientAuthInfo(const std::string& clientName) override;

    // 获取所有客户名称列表
    std::vector<std::string> getAllClientNames() override;

    // 获取指定客户和外壳号的授权信息
    std::vector<Authorization> getShellAuthorizationInfo(const std::string& clientName, const std::string& shellNumber) override;

    std::vector<std::string> selectAuthorizationByEncryptionKey(std::string encryptionKey) override;

    // 根据授权代码查询对应的外壳号列表
    std::vector<std::string> selectShellsByAuthorizationCode(const std::string& authorizationCode, const std::string& clientName);

    // 获取所有客户的suffix字段，返回客户名和对应的后缀对
    std::vector<std::pair<std::string, std::string>> getAllClientSuffixes();

    // 获取客户的授权信息（按授权ID分组）
    std::vector<std::vector<std::string>> getCustomerAuthorizationsByGroup(const std::string& clientName) override;

    // 获取指定客户的模型版本历史记录（包括最新版本和历史版本）
    nlohmann::json selectCustomerModelVersionHistory(const std::string& clientName) override;

    ~CustomerInfoDAO();

private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};

