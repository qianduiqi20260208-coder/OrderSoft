#include "CustomerInfoService.h"
#include <algorithm>
#include <map>
#include <ctime>
#include "Logger.h"

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

nlohmann::json CustomerInfoService::getClientAuthInfoJson(const std::string& clientName)
{
    nlohmann::json result;

    // 设置默认返回结构
    result["error"] = "";
    result["status"] = 1;
    result["data"] = nlohmann::json::object();

    // 调用DAO层获取客户信息
    Client client = customerInfoDAO_->getClientAuthInfo(clientName);

    // 如果没有找到客户信息或加密狗信息为空
    if (client.clientName.empty() || client.shellNumbers.empty())
    {
        return result;
    }

    // 构建返回的JSON数据
    nlohmann::json& data = result["data"];
    data["clientName"] = client.clientName;
    data["dongleCount"] = client.dongleCount;
    data["shellNumbers"] = nlohmann::json::array();

    // 处理每个加密狗信息
    for (const auto& shellNumber : client.shellNumbers)
    {
        nlohmann::json shellInfo;
        shellInfo["shellNumber"] = shellNumber.shellNumber;
        shellInfo["deviceType"] = shellNumber.deviceType;
        shellInfo["deviceNote"] = shellNumber.deviceNote;
        shellInfo["outTime"] = shellNumber.outTime;
        shellInfo["authCount"] = shellNumber.authCount;
        shellInfo["authorizationList"] = nlohmann::json::array();

        // 处理每个授权信息
        for (const auto& auth : shellNumber.authorizationList)
        {
            nlohmann::json authInfo;
            authInfo["authId"] = auth.authId;
            authInfo["startDate"] = auth.startDate;
            authInfo["endDate"] = auth.endDate;
            authInfo["authType"] = auth.authType;
            authInfo["authNote"] = auth.authNote;

            // 根据日期判断授权状态
            std::string status = "有效";

            // 获取当前日期（简化处理，实际应该使用标准库的日期时间函数）
            time_t now = time(nullptr);
            struct tm* timeinfo = localtime(&now);
            char currentDate[11];
            strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
            std::string currentDateStr(currentDate);

            // 如果结束日期小于当前日期，则状态为"过期"
            if (!auth.endDate.empty() && auth.endDate < currentDateStr)
            {
                status = "过期";
            }
            // 如果结束日期在当前日期的5天内，则状态为"临期"
            else if (!auth.endDate.empty())
            {
                // 计算结束日期与当前日期的差值（简化处理）
                // 这里只是简单地比较字符串，实际应该计算真实的日期差值
                time_t endTime;
                struct tm endTimeinfo = {0};
                if (sscanf(auth.endDate.c_str(), "%d-%d-%d", &endTimeinfo.tm_year, &endTimeinfo.tm_mon, &endTimeinfo.tm_mday) == 3)
                {
                    endTimeinfo.tm_year -= 1900; // 年份需要减去1900
                    endTimeinfo.tm_mon -= 1;     // 月份从0开始
                    endTime = mktime(&endTimeinfo);

                    // 计算日期差值（秒数）
                    double seconds = difftime(endTime, now);
                    double days = seconds / (60 * 60 * 24);

                    if (days <= 5 && days > 0)
                    {
                        status = "临期";
                    }
                }
            }

            authInfo["status"] = status;
            shellInfo["authorizationList"].push_back(authInfo);
        }

        data["shellNumbers"].push_back(shellInfo);
    }

    return result;
}

nlohmann::json CustomerInfoService::getClientList()
{
    nlohmann::json result;
    result["error"] = "";
    result["status"] = 1;
    result["data"] = nlohmann::json::array();

    try {
        // 获取所有客户名称
        std::vector<std::string> clientNames = customerInfoDAO_->getAllClientNames();

        // 获取当前日期用于授权状态判断
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        char currentDate[11];
        strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);
        std::string currentDateStr(currentDate);

        // 计算7天后的日期用于临期判断
        time_t sevenDaysLater = now + (7 * 24 * 60 * 60);
        struct tm* sevenDaysTimeinfo = localtime(&sevenDaysLater);
        char sevenDaysDate[11];
        strftime(sevenDaysDate, sizeof(sevenDaysDate), "%Y-%m-%d", sevenDaysTimeinfo);
        std::string sevenDaysDateStr(sevenDaysDate);

        for (const std::string& clientName : clientNames) {
            nlohmann::json clientData;
            clientData["clientName"] = clientName;

            // 获取客户详细信息
            Client client = customerInfoDAO_->getClientAuthInfo(clientName);

            // 设置基本信息
            clientData["dongleCount"] = client.dongleCount;
            clientData["clientinfo"] = ""; // 客户信息备注，当前数据结构中没有此字段

            // 统计模型和版本数量
            std::pair<int, int> modelStats = customerInfoDAO_->selectModelAndModelVersionCountByClient(clientName);
            clientData["modelCount"] = modelStats.first;
            clientData["modelVersionCount"] = modelStats.second;

            // 初始化授权统计
            int validCount = 0;
            int expiringCount = 0;
            int expiredCount = 0;

            // 遍历所有外壳号的授权信息进行统计
            for (const auto& shellNumber : client.shellNumbers) {
                for (const auto& auth : shellNumber.authorizationList) {
                    if (auth.endDate.empty()) {
                        // 如果没有结束日期，认为是永久有效
                        validCount++;
                    } else if (auth.endDate < currentDateStr) {
                        // 已过期
                        expiredCount++;
                    } else if (auth.endDate <= sevenDaysDateStr) {
                        // 7天内过期（临期）
                        expiringCount++;
                    } else {
                        // 有效
                        validCount++;
                    }
                }
            }

            // 设置授权统计信息
            nlohmann::json licenseStats;
            licenseStats["validCount"] = validCount;
            licenseStats["expiringCount"] = expiringCount;
            licenseStats["expiredCount"] = expiredCount;
            clientData["licenseStats"] = licenseStats;

            result["data"].push_back(clientData);
        }

    } catch (const std::exception& e) {
        result["error"] = std::string("获取客户列表失败: ") + e.what();
        result["status"] = 1;
        result["data"] = nlohmann::json::array();
    }

    return result;
}

std::vector<std::string> CustomerInfoService::getAllClientNames()
{
    return customerInfoDAO_->getAllClientNames();
}

std::vector<std::string> CustomerInfoService::getEncryptionKeyByClient(const std::string &clientName)
{
    // 调用DAO层获取指定客户的加密狗列表
    return customerInfoDAO_->selectEncryptionKeyByClient(clientName);
}

std::vector<Authorization> CustomerInfoService::getShellAuthorization(const std::string &clientName, const std::string &shellNumber)
{
    std::vector<Authorization> authorizationList = customerInfoDAO_->getShellAuthorizationInfo(clientName, shellNumber);
    return authorizationList;
}

nlohmann::json CustomerInfoService::getShellAuthorizationInfoJson(const std::string& clientName, const std::string& shellNumber)
{
    nlohmann::json result;

    // 设置默认返回结构
    result["error"] = "";
    result["status"] = 1;
    result["data"] = nlohmann::json::object();

    // 调用DAO层获取指定外壳号的授权信息
    std::vector<Authorization> authorizationList = customerInfoDAO_->getShellAuthorizationInfo(clientName, shellNumber);

    // 构建返回的JSON数据
    nlohmann::json& data = result["data"];
    data["clientName"] = clientName;
    data["shellNumber"] = shellNumber;
    data["authCount"] = authorizationList.size();
    data["authorizationList"] = nlohmann::json::array();

    // 处理每个授权信息
    for (const auto& auth : authorizationList)
    {
        nlohmann::json authInfo;
        authInfo["authId"] = auth.authId;
        authInfo["startDate"] = auth.startDate;
        authInfo["endDate"] = auth.endDate;
        authInfo["authType"] = auth.authType;
        authInfo["authNote"] = auth.authNote;

        // 根据日期判断授权状态
        std::string status = "有效";

        // 获取当前日期（简化处理，实际应该使用标准库的日期时间函数）
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        char currentDate[11];
        strftime(currentDate, sizeof(currentDate), "%Y-%m-%d", timeinfo);

        // 简单的字符串比较来判断日期（实际项目中应该使用更严格的日期比较）
        if (!auth.endDate.empty() && auth.endDate < std::string(currentDate))
        {
            status = "已过期";
        }

        authInfo["status"] = status;
        data["authorizationList"].push_back(authInfo);
    }

    return result;
}
