#pragma once
#include <vector>
#include <string>


class ICustomerInfoService
{
protected:
    typedef std::vector<std::string> SendRecord;
    typedef std::vector<std::string> SendOverview;
public:

    virtual std::vector<std::pair<std::vector<std::string>,std::vector<int>>> getClientGoodsCount() = 0;
    
    virtual bool addClientInfo(std::string,std::string) = 0;

    virtual bool alterClientInfo(std::string,std::string,std::string) = 0;


    virtual std::vector<std::pair<SendOverview,std::vector<SendRecord>>> getSendRecordByClientPagedByDate(std::string,int,int) = 0;

    virtual int getAllSendRecordGroupedCountByClient(std::string client) =0;

    virtual std::vector<std::vector<std::string>> getAllModelLatestVesrionByClient(std::string client) =0;
    virtual ~ICustomerInfoService() = default;
};

