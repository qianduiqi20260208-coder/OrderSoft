#include "EncryptionKeyService.h"

EncryptionKeyService::EncryptionKeyService(std::shared_ptr<IEncryptionKey> dao) : encryptionKeyDAO_(dao)
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
bool EncryptionKeyService::deliveryOperation(const std::string& clientName,
                                           const std::string& shellNumber,
                                           const std::string& deviceType,
                                           const std::string& deviceNote)
{
    // 调用DAO层的交付操作方法
    return encryptionKeyDAO_->deliveryOperation(clientName, shellNumber, deviceType, deviceNote);
}

bool EncryptionKeyService::returnOperation(const std::string& clientName,
                                           const std::string& shellNumber,
                                           const std::string& operationType,
                                           const std::string& inTime,
                                           const std::string& outTime,
                                           const std::string& remark)
{
    // 调用DAO层的归还操作方法
    return encryptionKeyDAO_->returnOperation(clientName, shellNumber, operationType, inTime, outTime, remark);

}

std::vector<std::pair<int, std::string>> EncryptionKeyService::getAvailableShellNumbers()
{
    // 调用DAO层的获取可交付外壳号列表方法
    return encryptionKeyDAO_->getAvailableShellNumbers();
}

bool EncryptionKeyService::createAuthorization(const std::string& clientName,
                                               const std::string& shellNumber,
                                               const std::string& authId,
                                               const std::string& authType,
                                               const std::string& startDate,
                                               const std::string& endDate,
                                               const std::string& authNote)
{
    // 调用DAO层的创建授权方法
    return encryptionKeyDAO_->createAuthorization(clientName, shellNumber, authId, authType, startDate, endDate, authNote);
}

bool EncryptionKeyService::updateAuthorizationEndDates(const std::string& clientName,
                                                       const std::vector<std::tuple<std::string, std::string, std::string>>& changes)
{
    // 调用DAO层的批量更新授权截止日期方法
    return encryptionKeyDAO_->updateAuthorizationEndDates(clientName, changes);
}

bool EncryptionKeyService::updateShellDeviceInfo(const std::string& clientName,
                                                 const std::string& shellNumber,
                                                 const std::string& deviceType,
                                                 const std::string& deviceNote)
{
    return encryptionKeyDAO_->updateShellDeviceInfo(clientName, shellNumber, deviceType, deviceNote);
}

EncryptionKeyService::~EncryptionKeyService()
{
}