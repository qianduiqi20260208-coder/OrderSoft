#pragma once
#include "ICustomerInfoService.h"
#include <memory>
#include "ICustomerInfoDAO.h"

class CustomerInfoService : public ICustomerInfoService
{
public:
    CustomerInfoService(std::shared_ptr<ICustomerInfoDAO> sp):customerInfoDAO_(sp){}
    //获取客户拥有的物品的数量，最外面的vector存放的是所有客户的情况，包裹的pair.first存放的是客户信息，pair.second存放的是客户拥有的物品的数量(依次是模型数量、模型版本数量、加密狗数量、有效授权数量、临期授权数量、过期授权数量)
    std::vector<std::pair<std::vector<std::string>,std::vector<int>>> getClientGoodsCount() override;

    bool addClientInfo(std::string s1,std::string s2) override { return customerInfoDAO_->createClient(s1,s2); }

    bool alterClientInfo(std::string originClient,std::string newClient,std::string clientInfo) override { return customerInfoDAO_->updateClient(originClient,newClient,clientInfo); }

    //最外面vector中的元素是每一天的记录 pair.first里发送总览也是发送日期以及发送数量 pair.send里的信息是具体的发送记录（这是一个数组）
    std::vector<std::pair<SendOverview,std::vector<SendRecord>>> getSendRecordByClientPagedByDate(std::string,int,int) override;

    //根据客户获取所有的发送记录的数量（按日期分组）
    int getAllSendRecordGroupedCountByClient(std::string client) override;

    //根据客户获取各个模型的最新版本
    std::vector<std::vector<std::string>> getAllModelLatestVesrionByClient(std::string client) override;
    // bool addClientInfo(std::string s1,std::string s2) override { customerInfoDAO_->createClient(s1,s2); };

    // bool alterClientInfo(std::string originClient,std::string newClient,std::string clientInfo) override { customerInfoDAO_->updateClient(originClient,newClient,clientInfo); };

    // 根据客户名称获取客户信息和加密狗授权信息，并转换为JSON格式
    nlohmann::json getClientAuthInfoJson(const std::string& clientName) override;

    // 获取所有客户名称列表
    std::vector<std::string> getAllClientNames() override;

    // 获取指定客户下的外壳号
    std::vector<std::string> getEncryptionKeyByClient(const std::string& clientName) override;

    // 获取指定客户、指定外壳号下的授权信息
    std::vector<Authorization> getShellAuthorization(const std::string& clientName, const std::string& shellNumber) override;

    // 获取客户的所有授权ID列表
    std::vector<Authorization> getCustomerAllAuthorizations(const std::string& clientName) override;

    // 根据授权ID获取对应的外壳号
    std::string getShellByAuthId(const std::string& authId, const std::string& clientName) override;

    // 根据授权ID获取对应的外壳号列表
    std::vector<std::string> getShellListByAuthId(const std::string& authId, const std::string& clientName) override;

    // 获取客户的授权信息（按授权ID分组）
    nlohmann::json getCustomerAuthorizationsByGroup(const std::string& clientName) override;

    // 获取指定客户和外壳号的授权信息
    nlohmann::json getShellAuthorizationInfoJson(const std::string& clientName, const std::string& shellNumber) override;

    // 获取客户列表及统计信息
    nlohmann::json getClientList() override;

    // 获取所有客户的suffix字段，返回带有客户标识的后缀列表
    std::vector<std::pair<std::string, std::string>> getAllClientSuffixList();

    // 获取指定客户的模型ATA章节号和发送模型数量
    std::pair<int,int> getModelAndModelVersionCountByClient(const std::string& clientName) override;

private:
    std::shared_ptr<ICustomerInfoDAO> customerInfoDAO_;
    std::vector<std::pair<SendOverview,std::vector<SendRecord>>> getAllSendRecordGroupedByClient(std::string client);
};