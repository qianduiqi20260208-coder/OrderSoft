#pragma once
#include <vector>
#include <string>
#include "Entity.h"
#include <memory>
#include <map>
#include <optional>

class IClientAuthService
{
public:
    virtual Client getAuthDetail(int clientId){return {};}

};

inline IClientAuthService::~IClientAuthService() = default;