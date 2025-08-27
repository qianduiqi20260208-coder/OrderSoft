#pragma once
#include "Entity.h"
#include <vector>
#include <string>
#include <nlohmann/json.hpp>


class ICustomerInfoService
{
protected:
    typedef std::vector<std::string> SendRecord;
    typedef std::vector<std::string> SendOverview;
public:

    virtual std::vector<std::pair<std::vector<std::string>,std::vector<int>>> getClientGoodsCount() = 0;
    
    virtual bool addClientInfo(std::string,std::string) = 0;

    // virtual bool addClientInfo(std::string,std::string) = 0;

    // virtual bool alterClientInfo(std::string,std::string,std::string) = 0;

    // 根据客户名称获取客户信息和加密狗授权信息，并转换为JSON格式
    virtual nlohmann::json getClientAuthInfoJson(const std::string& clientName) = 0;

    // 获取所有客户名称列表
    virtual std::vector<std::string> getAllClientNames() = 0;

    virtual bool alterClientInfo(std::string,std::string,std::string) = 0;

    // 获取指定客户和外壳号的授权信息
    virtual nlohmann::json getShellAuthorizationInfoJson(const std::string& clientName, const std::string& shellNumber) = 0;

    // 获取客户列表及统计信息
    virtual nlohmann::json getClientList() = 0;

    // 获取指定客户下的外壳号
    virtual std::vector<std::string> getEncryptionKeyByClient(const std::string& clientName) = 0;

    // 获取指定客户、指定外壳号下的授权信息
    virtual std::vector<Authorization> getShellAuthorization(const std::string& clientName, const std::string& shellNumber) = 0;

    virtual std::vector<std::pair<SendOverview,std::vector<SendRecord>>> getSendRecordByClientPagedByDate(std::string,int,int) = 0;

    virtual int getAllSendRecordGroupedCountByClient(std::string client) =0;

    virtual std::vector<std::vector<std::string>> getAllModelLatestVesrionByClient(std::string client) =0;

    // 获取所有客户的suffix字段，返回带有客户标识的后缀列表
    virtual std::vector<std::pair<std::string, std::string>> getAllClientSuffixList() = 0;

    virtual ~ICustomerInfoService() = default;
};

