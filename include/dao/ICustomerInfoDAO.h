#pragma once
#include "Entity.h"
#include <vector>
#include <memory>
#include <string>


class ICustomerInfoDAO
{
public:

    virtual bool createClient(std::string,std::string) = 0;
    virtual bool updateClient(std::string,std::string,std::string) = 0;

    virtual std::vector<std::pair<std::string,std::string>> selectAllClientInfo() = 0;
    virtual std::vector<std::string> selectEncryptionKeyByClient(std::string) = 0;
    virtual std::pair<int,int> selectModelAndModelVersionCountByClient(std::string) = 0;
    virtual std::vector<int> selectAuthorizationCountByEncryptionKey(std::string) = 0;

    // 根据客户名称获取客户信息和加密狗授权信息
    virtual Client getClientAuthInfo(const std::string& clientName) = 0;

    // 获取所有客户名称列表
    virtual std::vector<std::string> getAllClientNames() = 0;

    virtual ~ICustomerInfoDAO() = default;
};