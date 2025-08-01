#pragma once
#include <vector>
#include <string>
#include "Entity.h"
#include <memory>
#include <map>


class IUserService
{
public:
    virtual User getUserByJobNumber(int jobNumber){return {};}
    virtual std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber){return {};}
    
    virtual std::map<std::string,std::vector<int>> getOrderRole()= 0;
    virtual ~IUserService() =0;
};

inline IUserService::~IUserService() = default;