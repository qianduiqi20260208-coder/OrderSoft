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
