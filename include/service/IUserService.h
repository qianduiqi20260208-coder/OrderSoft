#pragma once
#include <vector>
#include <string>
#include "Entity.h"
#include <memory>


class IUserService
{
public:
    virtual User getUserByJobNumber(int jobNumber){return {};}
    virtual std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber){return {};}

    virtual ~IUserService() =0;
};

inline IUserService::~IUserService() = default;