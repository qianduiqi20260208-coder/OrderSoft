#pragma once
#include <vector>
#include <string>
#include "Entity.h"
#include <memory>
#include <map>
#include <optional>

class IUserService
{
public:
    virtual User getUserByJobNumber(int jobNumber){return {};}
    virtual std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber){return {};}
    
    virtual std::map<std::string,std::vector<int>> getOrderRole()= 0;
    virtual std::map<std::string, std::vector<std::pair<int, std::string>>> getOrderRole_(const std::string& modelName) = 0;  // 新方法

    virtual std::optional<User> login(const std::string& account, const std::string& password) = 0;

    virtual ~IUserService() =0;
};

inline IUserService::~IUserService() = default;