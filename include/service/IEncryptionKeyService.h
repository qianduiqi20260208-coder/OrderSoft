#pragma once
#include <vector>
#include <string>
#include "Entity.h"


class IEncryptionKeyService
{
protected:
    typedef std::vector<std::string> EncryptionKeyHistory;
    typedef std::vector<std::string> AuthInfo;
public:
    virtual std::vector<DongleInfo> getDongleInfo() = 0;
    virtual bool createEncryptionKey(std::string,std::string) = 0;
    virtual bool updateEncryptionKey(int id,std::string,std::string) = 0;

    virtual std::vector<std::pair<EncryptionKeyHistory,std::vector<AuthInfo>>> getEncryptionKeyHistoryWithAuthRecordByEK(std::string) = 0;

    virtual ~IEncryptionKeyService() = default;
};