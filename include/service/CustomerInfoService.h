#pragma once
#include "ICustomerInfoService.h"
#include <memory>
#include "ICustomerInfoDAO.h"

class CustomerInfoService : public ICustomerInfoService
{
public:
    CustomerInfoService(std::shared_ptr<ICustomerInfoDAO> sp):customerInfoDAO_(sp){}
    //获取客户拥有的物品的数量 返回值pair.first存放的是客户信息 pair.second存放的是客户拥有的物品的数量
    std::pair<std::vector<std::string>,std::vector<int>> getClientGoodsCount() override;

    // bool addClientInfo(std::string s1,std::string s2) override { customerInfoDAO_->createClient(s1,s2); };

    // bool alterClientInfo(std::string originClient,std::string newClient,std::string clientInfo) override { customerInfoDAO_->updateClient(originClient,newClient,clientInfo); };

    // 根据客户名称获取客户信息和加密狗授权信息，并转换为JSON格式
    nlohmann::json getClientAuthInfoJson(const std::string& clientName) override;

    // 获取所有客户名称列表
    std::vector<std::string> getAllClientNames() override;

    // 获取指定客户和外壳号的授权信息
    nlohmann::json getShellAuthorizationInfoJson(const std::string& clientName, const std::string& shellNumber) override;

private:
    std::shared_ptr<ICustomerInfoDAO> customerInfoDAO_;
};