#pragma once#pragma once
#include "Entity.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

class ICustomerInfoService
{
public:
    // 根据客户名称获取客户信息和加密狗授权信息，并转换为JSON格式
    virtual nlohmann::json getClientAuthInfoJson(const std::string& clientName) = 0;
    
    // 获取所有客户名称列表
    virtual std::vector<std::string> getAllClientNames() = 0;
    
    virtual ~ICustomerInfoService() = 0;
};

inline ICustomerInfoService::~ICustomerInfoService() = default;