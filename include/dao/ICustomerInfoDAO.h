#pragma once
#include "Entity.h"
#include <vector>
#include <memory>
#include <string>

class ICustomerInfoDAO
{
public:
    // 根据客户名称获取客户信息和加密狗授权信息
    virtual Client getClientAuthInfo(const std::string& clientName) = 0;
    
    // 获取所有客户名称列表
    virtual std::vector<std::string> getAllClientNames() = 0;
    
    virtual ~ICustomerInfoDAO() = 0;
};

inline ICustomerInfoDAO::~ICustomerInfoDAO() = default;