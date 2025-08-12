#pragma once


class ICustomerInfoDAO
{
public:

    virtual bool createClient(std::string,std::string) = 0;
    virtual bool updateClient(std::string,std::string,std::string) = 0;

    virtual std::vector<std::pair<std::string,std::string>> selectAllClientInfo() = 0;
    virtual std::vector<const char*> selectEncryptionKeyByClient(std::string) = 0;
    virtual std::pair<int,int> selectModelAndModelVersionCountByClient(std::string) = 0;
    virtual std::vector<int> selectAuthorizationCountByEncryptionKey(std::string) = 0;


    virtual ~ICustomerInfoDAO() = default;
};