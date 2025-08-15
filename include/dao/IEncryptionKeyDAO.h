#pragma once
#include "Entity.h"
#include <vector>

class IEncryptionKey
{
public:
    virtual std::vector<DongleInfo> getDongleInfo() = 0;
    virtual ~IEncryptionKey() = 0;
    virtual bool createEncryptionKey(std::string,std::string) = 0;
    virtual bool updateEncryptionKey(int id,std::string,std::string) = 0;

    virtual std::vector<std::vector<std::string>> selectAllEncryptionHistoryByEK(std::string) = 0;

    virtual std::vector<std::vector<std::string>> selectAllAuthInfoByClientEK(std::string,std::string) = 0;


};

inline IEncryptionKey::~IEncryptionKey() = default;
