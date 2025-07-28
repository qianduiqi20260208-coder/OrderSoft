#pragma once
#include <vector>
#include "Entity.h"
#include <memory>

class IUserDAO
{
public:
    virtual std::vector<User> getUser(){return {};}
    virtual std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber){return {};}

    virtual ~IUserDAO() = 0; 
};

inline IUserDAO::~IUserDAO() =default;