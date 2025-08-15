#include "CustomerInfoService.h"
#include <algorithm>

std::vector<std::pair<std::vector<std::string>, std::vector<int>>> CustomerInfoService::getClientGoodsCount()
{
    std::vector<std::pair<std::vector<std::string>, std::vector<int>>> retVec;

    const auto& clientVec = customerInfoDAO_->selectAllClientInfo();
    for(const auto& client: clientVec) 
    {
        std::pair<std::vector<std::string>, std::vector<int>> pr;
        //添加客户以及客户备注
        pr.first.push_back(client.first);
        pr.first.push_back(client.second);

        //计算出模型以及模型版本的数量 添加到int类型的vector
        const std::pair<int, int>& modelAndModelVersionCount = customerInfoDAO_->selectModelAndModelVersionCountByClient(client.first);
        pr.second.push_back(modelAndModelVersionCount.first);
        pr.second.push_back(modelAndModelVersionCount.second);

        const std::vector<std::string>& encryptionKeys = customerInfoDAO_->selectEncryptionKeyByClient(client.first);
        //添加加密狗的数量
        pr.second.push_back(encryptionKeys.size());

        int validAuthCount = 0;
        int nearAuthCount = 0;
        int expireAuthCount = 0;

        for(const auto& encryptionKey : encryptionKeys)
        {
            const std::vector<int>& authorizationCount = customerInfoDAO_->selectAuthorizationCountByEncryptionKey(encryptionKey);
            if(authorizationCount.size() == 3)
            {
                validAuthCount += authorizationCount[0];
                nearAuthCount += authorizationCount[1];
                expireAuthCount += authorizationCount[2];
            }else{
                printf("function:getClientGoodsCount() error:授权数量数组的大小不等于3！\n");
                break;
            }

        }
        pr.second.push_back(validAuthCount);
        pr.second.push_back(nearAuthCount);
        pr.second.push_back(expireAuthCount);

        retVec.push_back(pr);
    }

    return retVec;
}

std::vector<std::pair<ICustomerInfoService::SendOverview, std::vector<ICustomerInfoService::SendRecord>>> CustomerInfoService::getSendRecordByClientPagedByDate(std::string client, int page, int pageSize)
{

    const auto& allSendRecordGrouped =  getAllSendRecordGroupedByClient(client);
    //筛选出需要的组
    std::vector<std::pair<ICustomerInfoService::SendOverview, std::vector<ICustomerInfoService::SendRecord>>> retVec;
    size_t offset = page * pageSize;

    //偏移量不合法直接返回空
    if(offset >= allSendRecordGrouped.size())
    {
        return {};
    }
    size_t end = std::min(offset + pageSize,allSendRecordGrouped.size());
    
    return {allSendRecordGrouped.begin()+offset,allSendRecordGrouped.begin()+end};
}

int CustomerInfoService::getAllSendRecordGroupedCountByClient(std::string client)
{
    return getAllSendRecordGroupedByClient(client).size();
}

std::vector<std::vector<std::string>> CustomerInfoService::getAllModelLatestVesrionByClient(std::string client)
{
    //首先查出来发送给这个客户的有哪些模型

    //然后再去查询各个模型的最新版版本 根据工单创建时间来查询
    return customerInfoDAO_->selectLatestModelVersionByClient(client);
}

std::vector<std::pair<ICustomerInfoService::SendOverview, std::vector<ICustomerInfoService::SendRecord>>> CustomerInfoService::getAllSendRecordGroupedByClient(std::string client)
{
    //将所有工单分组
    std::vector<std::pair<ICustomerInfoService::SendOverview, std::vector<ICustomerInfoService::SendRecord>>> groups;
    const std::vector<std::vector<std::string>>& allSendRecord = customerInfoDAO_->selectAllSendRecordByClient(client);

    for(const auto& sendRecord : allSendRecord)
    {
        std::string date = sendRecord[3].substr(0,10);
        if(groups.empty() || groups.back().first[0] != date)
        {
            if(!groups.empty())
            //先填入前一个分组的数量 然后再创建分组
                groups.back().first.push_back(std::to_string((groups.back().second.size())));
            
            //创建新的分组
            std::pair<ICustomerInfoService::SendOverview, std::vector<ICustomerInfoService::SendRecord>> pr;
            pr.first.push_back(date);
            groups.push_back(pr);
        }

        groups.back().second.push_back({sendRecord[0],sendRecord[1],sendRecord[2],sendRecord[3]});
    }



    return groups;
}
