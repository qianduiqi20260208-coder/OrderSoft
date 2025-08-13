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

    bool addClientInfo(std::string s1,std::string s2) override { customerInfoDAO_->createClient(s1,s2); };

    bool alterClientInfo(std::string originClient,std::string newClient,std::string clientInfo) override { customerInfoDAO_->updateClient(originClient,newClient,clientInfo); };
private:
    std::shared_ptr<ICustomerInfoDAO> customerInfoDAO_;
};