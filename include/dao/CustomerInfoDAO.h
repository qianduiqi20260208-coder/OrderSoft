#pragma once
#include "ICustomerInfoDAO.h"
#include <mysql.h>
#define SQL_MAX 512		// sql语句字符数组最大值

class CustomerInfoDAO : public ICustomerInfoDAO
{
public:
    CustomerInfoDAO(MYSQL* m);
    
    bool createClient(std::string,std::string) override;
    //第一个string是原客户名称，第二个string是新客户名称，第三个string是客户信息备注
    bool updateClient(std::string,std::string,std::string) override;

    std::vector<std::pair<std::string,std::string>> selectAllClientInfo() override;
    std::vector<std::string> selectEncryptionKeyByClient(std::string) override;
    std::pair<int,int> selectModelAndModelVersionCountByClient(std::string) override;
    std::vector<int> selectAuthorizationCountByEncryptionKey(std::string) override;

    ~CustomerInfoDAO();
private:
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
};

