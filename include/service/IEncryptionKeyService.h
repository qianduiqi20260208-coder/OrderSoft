#pragma once
#include <vector>
#include <string>
#include "Entity.h"

class IEncryptionKeyService
{
public:
    virtual std::vector<DongleInfo> getDongleInfo() = 0;
    virtual bool createEncryptionKey(std::string,std::string) = 0;
    virtual bool updateEncryptionKey(int id,std::string,std::string) = 0;
    virtual ~IEncryptionKeyService() = default;
};