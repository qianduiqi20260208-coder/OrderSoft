#pragma once
#include <vector>
#include "Entity.h"
#include <memory>

class IUserDAO
{
public:
    virtual std::vector<User> getUser() =0;
    virtual std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber)=0;

    virtual std::vector<int> getOrderApprover() = 0;
    virtual std::vector<std::pair<int, std::string>> getOrderApprover_() = 0;
    virtual std::vector<int> getOrderDispatcher() = 0;
    virtual std::vector<std::pair<int, std::string>> getOrderDispatcher_() = 0;
    virtual std::vector<int> getOrderExecutor() = 0;
    virtual std::vector<std::pair<int, std::string>> getOrderExecutor_() = 0;

    virtual bool login(std::string account,std::string password) = 0;

    virtual std::vector<std::string> getUserResponsibleModels(int userId) = 0;


    virtual ~IUserDAO() = 0; 
};

inline IUserDAO::~IUserDAO() =default;