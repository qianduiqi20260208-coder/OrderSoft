#include "CustomerInfoService.h"
#include <map>
#include <ctime>

std::pair<std::vector<std::string>, std::vector<int>> CustomerInfoService::getClientGoodsCount()
{
    //customerInfoDAO_->selectEncryptionKeyByClient()
    return std::pair<std::vector<std::string>, std::vector<int>>();
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
        result["error"] = "未找到客户信息或该客户没有加密狗授权信息";
        result["status"] = 0;
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
        shellInfo["inTime"] = shellNumber.inTime;
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

std::vector<std::string> CustomerInfoService::getAllClientNames()
{
    return customerInfoDAO_->getAllClientNames();
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