#pragma once
#include "IEncryptionKeyService.h"
#include "IEncryptionKeyDAO.h"
#include <memory>

class EncryptionKeyService : public IEncryptionKeyService
{
public:
    EncryptionKeyService(std::shared_ptr<IEncryptionKey>);
    std::vector<DongleInfo> getDongleInfo() override;
    bool createEncryptionKey(std::string,std::string) override;
    bool updateEncryptionKey(int id,std::string,std::string) override;

private:
    std::shared_ptr<IEncryptionKey> encryptionKeyDAO_;
};