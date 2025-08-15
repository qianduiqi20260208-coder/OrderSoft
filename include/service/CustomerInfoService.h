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

private:
    std::shared_ptr<ICustomerInfoDAO> customerInfoDAO_;
    std::vector<std::pair<SendOverview,std::vector<SendRecord>>> getAllSendRecordGroupedByClient(std::string client);
};