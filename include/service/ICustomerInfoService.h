#pragma once
#include <vector>
#include <string>

class ICustomerInfoService
{
public:

    virtual std::pair<std::vector<std::string>,std::vector<int>> getClientGoodsCount() = 0;
    
    virtual bool addClientInfo(std::string,std::string) = 0;

    virtual bool alterClientInfo(std::string,std::string,std::string) = 0;

    virtual ~ICustomerInfoService() = default;
};

