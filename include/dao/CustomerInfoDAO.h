#pragma once
#include "ICustomerInfoDAO.h"
#include <mysql.h>
#define SQL_MAX 1024	

class CustomerInfoDAO : public ICustomerInfoDAO
{
public:
    CustomerInfoDAO(MYSQL* ms);

    bool createClient(std::string,std::string) override;
    //第一个string是原客户名称，第二个string是新客户名称，第三个string是客户信息备注
    bool updateClient(std::string,std::string,std::string) override;

    std::vector<std::pair<std::string,std::string>> selectAllClientInfo() override;
    std::vector<std::string> selectEncryptionKeyByClient(std::string) override;
    std::pair<int,int> selectModelAndModelVersionCountByClient(std::string) override;
    std::vector<int> selectAuthorizationCountByEncryptionKey(std::string) override;

    // 根据客户名称获取客户信息和加密狗授权信息
    Client getClientAuthInfo(const std::string& clientName) override;

    // 获取所有客户名称列表
    std::vector<std::string> getAllClientNames() override;

    ~CustomerInfoDAO();

private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};