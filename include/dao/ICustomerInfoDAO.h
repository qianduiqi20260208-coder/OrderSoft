#pragma once
#include <string>
#include <vector>

class ICustomerInfoDAO
{
public:

    virtual bool createClient(std::string,std::string) = 0;
    virtual bool updateClient(std::string,std::string,std::string) = 0;

    virtual std::vector<std::pair<std::string,std::string>> selectAllClientInfo() = 0;
    virtual std::vector<std::string> selectEncryptionKeyByClient(std::string) = 0;
    virtual std::pair<int,int> selectModelAndModelVersionCountByClient(std::string) = 0;
    virtual std::vector<int> selectAuthorizationCountByEncryptionKey(std::string) = 0;


    virtual std::vector<std::vector<std::string>> selectAllSendRecordByClient(std::string) = 0;

    virtual std::vector<std::vector<std::string>> selectLatestModelVersionByClient(std::string client) =0;

    virtual ~ICustomerInfoDAO() = default;
};