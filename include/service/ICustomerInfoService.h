#pragma once
#include "Entity.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

class ICustomerInfoService
{
public:

    virtual std::pair<std::vector<std::string>,std::vector<int>> getClientGoodsCount() = 0;

    // virtual bool addClientInfo(std::string,std::string) = 0;

    // virtual bool alterClientInfo(std::string,std::string,std::string) = 0;

    // 根据客户名称获取客户信息和加密狗授权信息，并转换为JSON格式
    virtual nlohmann::json getClientAuthInfoJson(const std::string& clientName) = 0;

    // 获取所有客户名称列表
    virtual std::vector<std::string> getAllClientNames() = 0;

    // 获取指定客户和外壳号的授权信息
    virtual nlohmann::json getShellAuthorizationInfoJson(const std::string& clientName, const std::string& shellNumber) = 0;

    virtual ~ICustomerInfoService() = 0;

};

inline ICustomerInfoService::~ICustomerInfoService() = default;

