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
        //添加加密锁的数量
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

    // 如果没有找到客户信息或加密锁信息为空
    if (client.clientName.empty() || client.shellNumbers.empty())
    {
        return result;
    }

    // 构建返回的JSON数据
    nlohmann::json& data = result["data"];
    data["clientName"] = client.clientName;
    data["dongleCount"] = client.dongleCount;
    data["shellNumbers"] = nlohmann::json::array();

    // 处理每个加密锁信息
    for (const auto& shellNumber : client.shellNumbers)
    {
        nlohmann::json shellInfo;
        shellInfo["shellNumber"] = shellNumber.shellNumber;
        shellInfo["deviceType"] = shellNumber.deviceType;
        shellInfo["deviceNote"] = shellNumber.deviceNote;
        shellInfo["contractName"] = shellNumber.contractName;
        shellInfo["contractNumber"] = shellNumber.contractNumber;
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

nlohmann::json CustomerInfoService::getCustomerModelVersionHistory(const std::string& clientName)
{
    nlohmann::json result = {
        {"status", 1},
        {"error", ""},
        {"data", {
            {"clientName", clientName},
            {"models", nlohmann::json::array()}
        }}
    };

    try {
        // 调用DAO层获取客户模型版本历史数据
        nlohmann::json historyData = customerInfoDAO_->selectCustomerModelVersionHistory(clientName);
        
        // 构建返回数据结构
        nlohmann::json& data = result["data"];
        data["clientName"] = clientName;
        data["models"] = nlohmann::json::array();
        
        // 转换数据格式以匹配前端期望的结构
        if (!historyData.is_null() && historyData.is_array()) {
            for (const auto& model : historyData) {
                nlohmann::json transformedModel;
                
                // 映射基本字段（用于表格直接显示）
                transformedModel["ata_code"] = model["model_ata_code"];
                transformedModel["model_name"] = model["model_chinese_name"];
                transformedModel["latest_version"] = model["latest_version"];
                
                // 处理版本数组 - 创建两种格式
                if (model.contains("versions") && model["versions"].is_array()) {
                    // 格式1：用于对话框显示的扁平数组
                    transformedModel["versions"] = nlohmann::json::array();
                    
                    // 格式2：用于表格直接显示的version_info对象（每个版本一个键值）
                    transformedModel["version_info"] = nlohmann::json::object();
                    
                    // 按日期分组统计版本数量
                    std::map<std::string, int> dateGroupCount;
                    
                    // 第一遍：统计每个日期的版本数量
                    for (const auto& version : model["versions"]) {
                        std::string completedDate = version["completed_date"];
                        size_t spacePos = completedDate.find(' ');
                        std::string dateKey = (spacePos != std::string::npos) ? completedDate.substr(0, spacePos) : completedDate;
                        dateGroupCount[dateKey]++;
                    }
                    
                    // 第二遍：为每个版本创建唯一键值
                    std::map<std::string, int> currentDateIndex;
                    
                    for (const auto& version : model["versions"]) {
                        nlohmann::json transformedVersion;
                        
                        // 映射版本字段，使用前端期望的字段名
                        transformedVersion["modelName"] = model["model_chinese_name"];
                        transformedVersion["modelVersion"] = version["version"];
                        transformedVersion["workOrderNo"] = version["work_order_id"];
                        
                        // 提取日期部分（去掉时间）
                        std::string completedDate = version["completed_date"];
                        size_t spacePos = completedDate.find(' ');
                        std::string dateKey;
                        if (spacePos != std::string::npos) {
                            dateKey = completedDate.substr(0, spacePos);
                            transformedVersion["deliveryDate"] = dateKey;
                        } else {
                            dateKey = completedDate;
                            transformedVersion["deliveryDate"] = dateKey;
                        }
                        
                        // 设置发送日期（这里使用完成日期作为发送日期）
                        if (spacePos != std::string::npos) {
                            transformedVersion["packageSendDate"] = completedDate.substr(0, spacePos);
                        } else {
                            transformedVersion["packageSendDate"] = completedDate;
                        }
                        
                        transformedVersion["isLatest"] = version["is_latest"];
                        
                        // 添加到格式1数组
                        transformedModel["versions"].push_back(transformedVersion);
                        
                        // 为每个版本创建唯一的键值：如果同一日期有多个版本，添加索引后缀
                        int dateIndex = currentDateIndex[dateKey]++;
                        std::string versionKey = (dateIndex == 0 && dateGroupCount[dateKey] == 1) ? dateKey : dateKey + "_" + std::to_string(dateIndex);
                        
                        nlohmann::json versionInfo;
                        versionInfo["version_id"] = version["work_order_id"];
                        versionInfo["version"] = version["version"];
                        versionInfo["is_latest"] = version["is_latest"];
                        versionInfo["delivery_date"] = dateKey; // 保存基础日期用于前端匹配
                        versionInfo["date_index"] = dateIndex; // 保存日期内索引
                        
                        transformedModel["version_info"][versionKey] = versionInfo;
                    }
                }
                
                data["models"].push_back(transformedModel);
            }
        }
        
    } catch (const std::exception& e) {
        result["error"] = std::string("获取客户模型版本历史失败: ") + e.what();
        result["status"] = 0;
        result["data"] = nlohmann::json::object();
    }

    return result;
}

nlohmann::json CustomerInfoService::getCustomerAuthorizationsByGroup(const std::string& clientName)
{
    nlohmann::json result = {
        {"status", 1},
        {"error", ""},
        {"data", {
            {"list", nlohmann::json::array()}
        }}
    };
    
    try {
        // 获取原始数据
        std::vector<std::vector<std::string>> rawData = customerInfoDAO_->getCustomerAuthorizationsByGroup(clientName);
        
        // 按授权ID分组
        std::map<std::string, std::vector<std::vector<std::string>>> groupedData;
        
        for (const auto& row : rawData) {
            if (row.size() >= 8) {
                std::string authId = row[0]; // authorization_code
                if (!authId.empty()) {
                    groupedData[authId].push_back(row);
                }
            }
        }
        
        // 转换为指定格式
        nlohmann::json authList = nlohmann::json::array();
        
        for (const auto& group : groupedData) {
            nlohmann::json authItem;
            authItem["authId"] = group.first;
            authItem["shellNumberList"] = nlohmann::json::array();
            
            for (const auto& row : group.second) {
                nlohmann::json shellItem;
                
                
                shellItem["startTime"] = row[1]; // authorization_start_date
                shellItem["endTime"] = row[2]; // authorization_end_date
                shellItem["deviceType"] = row[3]; // encryption_type
                shellItem["description"] = row[4]; // remark
                shellItem["shellNumber"] = row[6]; // shell_number
                shellItem["authorizationId"] = row[7]; // authorization_id
                
                authItem["shellNumberList"].push_back(shellItem);
            }
            
            authList.push_back(authItem);
        }
        
        result["data"]["list"] = authList;
        
    } catch (const std::exception& e) {
        LOG_ERROR("getCustomerAuthorizationsByGroup 处理数据时发生异常: %s", e.what());
        result["status"] = 0;
        result["error"] = "处理数据时发生异常";
    }
    
    return result;
}

std::vector<Authorization> CustomerInfoService::getCustomerAllAuthorizations(const std::string& clientName)
{
    std::vector<Authorization> allAuthorizations;
    
    // 获取客户的所有外壳号
    std::vector<std::string> shellNumbers = customerInfoDAO_->selectEncryptionKeyByClient(clientName);
    
    // 遍历每个外壳号，获取其授权信息
    for (const auto& shellNumber : shellNumbers) {
        std::vector<Authorization> shellAuths = customerInfoDAO_->getShellAuthorizationInfo(clientName, shellNumber);
        allAuthorizations.insert(allAuthorizations.end(), shellAuths.begin(), shellAuths.end());
    }
    
    return allAuthorizations;
}

std::string CustomerInfoService::getShellByAuthId(const std::string& authId, const std::string& clientName)
{
    // 获取客户的所有外壳号
    std::vector<std::string> shellNumbers = customerInfoDAO_->selectEncryptionKeyByClient(clientName);
    
    // 遍历每个外壳号，查找匹配的授权ID
    for (const auto& shellNumber : shellNumbers) {
        std::vector<Authorization> shellAuths = customerInfoDAO_->getShellAuthorizationInfo(clientName, shellNumber);
        
        for (const auto& auth : shellAuths) {
            if (auth.authId == authId) {
                return shellNumber;
            }
        }
    }
    
    // 如果没有找到匹配的授权ID，返回空字符串
    return "";
}

std::vector<std::string> CustomerInfoService::getShellListByAuthId(const std::string& authId, const std::string& clientName)
{
    // 直接通过授权代码查询对应的外壳号列表
    return customerInfoDAO_->selectShellsByAuthorizationCode(authId, clientName);
}

std::vector<std::pair<std::string, std::string>> CustomerInfoService::getAllClientSuffixList()
{
    std::vector<std::pair<std::string, std::string>> result;
    
    // 从DAO层获取所有客户的suffix字段
    std::vector<std::pair<std::string, std::string>> clientSuffixes = customerInfoDAO_->getAllClientSuffixes();
    
    // 将每个逗号分隔的字符串拆分为单独的元素，并标识客户
    for (const auto& clientSuffix : clientSuffixes) {
        const std::string& clientName = clientSuffix.first;
        const std::string& suffixStr = clientSuffix.second;
        
        if (!suffixStr.empty()) {
            // 使用逗号分隔字符串
            std::string current = suffixStr;
            size_t pos = 0;
            while ((pos = current.find(',')) != std::string::npos) {
                std::string token = current.substr(0, pos);
                // 去除前后空格
                token.erase(0, token.find_first_not_of(" \t"));
                token.erase(token.find_last_not_of(" \t") + 1);
                if (!token.empty()) {
                    result.push_back(std::make_pair(clientName, token));
                }
                current.erase(0, pos + 1);
            }
            // 处理最后一个元素
            current.erase(0, current.find_first_not_of(" \t"));
            current.erase(current.find_last_not_of(" \t") + 1);
            if (!current.empty()) {
                result.push_back(std::make_pair(clientName, current));
            }
        }
    }
    
    return result;
}

std::pair<int, int> CustomerInfoService::getModelAndModelVersionCountByClient(const std::string &clientName)
{
    // 统计模型和版本数量
    std::pair<int, int> modelStats = customerInfoDAO_->selectModelAndModelVersionCountByClient(clientName);
    return modelStats;
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
            clientData["clientinfo"] = client.remarks; // 客户信息备注，当前数据结构中没有此字段

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
    // 调用DAO层获取指定客户的加密锁列表
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
