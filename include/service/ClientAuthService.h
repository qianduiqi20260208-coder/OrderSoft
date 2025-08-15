#pragma once
#include "IClientAuthService.h"
#include "IClientDAO.h"
#include <memory>
#include <nlohmann/json.hpp>

#include <vector>
#include <string>

class ClientAuthService :public IClientAuthService
{
public:
    ClientAuthService(std::shared_ptr<IClientDAO> sp);
    Client getAuthDetail(int clientId)override;

    
private:
    std::shared_ptr<IClientDAO> clientDAO_;
};