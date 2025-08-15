#include "EncryptionKeyService.h"

EncryptionKeyService::EncryptionKeyService(std::shared_ptr<IEncryptionKey> tmp):encryptionKeyDAO_(tmp)
{

}

std::vector<DongleInfo> EncryptionKeyService::getDongleInfo()
{

    return encryptionKeyDAO_->getDongleInfo();
}

bool EncryptionKeyService::createEncryptionKey(std::string s1, std::string s2)
{
    return encryptionKeyDAO_->createEncryptionKey(s1,s2);
}

bool EncryptionKeyService::updateEncryptionKey(int id, std::string s1, std::string s2)
{
    return encryptionKeyDAO_->updateEncryptionKey(id,s1,s2);
}

std::vector<std::pair<IEncryptionKeyService::EncryptionKeyHistory, std::vector<IEncryptionKeyService::AuthInfo>>> EncryptionKeyService::getEncryptionKeyHistoryWithAuthRecordByEK(std::string ek)
{
    std::vector<std::pair<IEncryptionKeyService::EncryptionKeyHistory, std::vector<IEncryptionKeyService::AuthInfo>>> retVec;
    //根据加密狗查出所有的历史记录 包括出库时间 入库时间 客户
    const auto& allEncryptionHistory =  encryptionKeyDAO_->selectAllEncryptionHistoryByEK(ek);
    //根据客户查找所有的所有的授权信息 
    for(const auto& singleEncryptionHis : allEncryptionHistory)
    {
        std::pair<IEncryptionKeyService::EncryptionKeyHistory, std::vector<IEncryptionKeyService::AuthInfo>> pr;
        //历史记录 包括出库时间 入库时间 客户信息
        pr.first.push_back(singleEncryptionHis[0]);
        pr.first.push_back(singleEncryptionHis[1]);
        pr.first.push_back(singleEncryptionHis[2]);

        const auto& customer = singleEncryptionHis[2];
        std::vector<std::vector<std::string>> allAuthInfo = encryptionKeyDAO_->selectAllAuthInfoByClientEK(customer,ek);
        pr.second.insert(pr.second.end(),allAuthInfo.begin(),allAuthInfo.end());

        retVec.push_back(pr);
    }

    return retVec;
}
