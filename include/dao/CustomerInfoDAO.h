#pragma once
#include "ICustomerInfoDAO.h"
#include <mysql.h>

class CustomerInfoDAO : public ICustomerInfoDAO
{
public:
    CustomerInfoDAO(MYSQL* ms);
    
    // 根据客户名称获取客户信息和加密狗授权信息
    Client getClientAuthInfo(const std::string& clientName) override;
    
    // 获取所有客户名称列表
    std::vector<std::string> getAllClientNames() override;
    
    ~CustomerInfoDAO();
    
private:
    // mysql套件
    MYSQL* mysql;
    char sql[1024]; // 存储sql语句
};