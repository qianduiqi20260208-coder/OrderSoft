#include "UserService.h"
#include <algorithm>

UserService::UserService(std::shared_ptr<IUserDAO> sp):userDAO_(sp)
{
}

User UserService::getUserByJobNumber(int jobNumber)
{
    User user;
    users_ = userDAO_->getUser();
    for(auto ele:users_)
    {
        if(ele.jobNumber == jobNumber)
            user = ele;
    }

    return user;
}

std::vector<std::shared_ptr<Ticket>> UserService::getUserOrder(int jobNumber)
{
    auto ret = userDAO_->getUserOrder(jobNumber);
    return ret;
}

std::map<std::string, std::vector<int>> UserService::getOrderRole()
{

    std::map<std::string, std::vector<int>> retMap;
    retMap["审批人"] = userDAO_->getOrderApprover();
    retMap["分发人"] = userDAO_->getOrderDispatcher();
    retMap["执行人"] = userDAO_->getOrderExecutor();

    return retMap;
}

// 修改方法，添加模型名参数，所有角色都根据模型筛选
std::map<std::string, std::vector<int>> UserService::getOrderRole_(const std::string& modelName)
{
    std::map<std::string, std::vector<int>> retMap;
    
    if (modelName.empty()) {
        // 如果没有传入模型名，返回所有角色的用户
        retMap["审批人"] = userDAO_->getOrderApprover();
        retMap["分发人"] = userDAO_->getOrderDispatcher();
        retMap["执行人"] = userDAO_->getOrderExecutor();
    } else {
        // // ✅ 所有角色都根据模型筛选
        // printf("[info] getOrderRole 开始筛选模型 %s 的相关角色\n", modelName.c_str());
        
        // 筛选审批人
        std::vector<int> filteredApprovers;
        std::vector<int> allApprovers = userDAO_->getOrderApprover();
        for (int jobNumber : allApprovers) {
            // ✅ 使用 std::find 简化逻辑
            std::vector<std::string> responsibleModels = userDAO_->getUserResponsibleModels(jobNumber);
            if (std::find(responsibleModels.begin(), responsibleModels.end(), modelName) != responsibleModels.end()) {
                filteredApprovers.push_back(jobNumber);
                // printf("[debug] 审批人 %d 负责模型 %s\n", jobNumber, modelName.c_str());
            }
        }
        retMap["审批人"] = filteredApprovers;
        
        // 筛选分发人
        std::vector<int> filteredDispatchers;
        std::vector<int> allDispatchers = userDAO_->getOrderDispatcher();
        for (int jobNumber : allDispatchers) {
            // ✅ 使用 std::find 简化逻辑
            std::vector<std::string> responsibleModels = userDAO_->getUserResponsibleModels(jobNumber);
            if (std::find(responsibleModels.begin(), responsibleModels.end(), modelName) != responsibleModels.end()) {
                filteredDispatchers.push_back(jobNumber);
                // printf("[debug] 分发人 %d 负责模型 %s\n", jobNumber, modelName.c_str());
            }
        }
        retMap["分发人"] = filteredDispatchers;
        
        // 筛选执行人
        std::vector<int> filteredExecutors;
        std::vector<int> allExecutors = userDAO_->getOrderExecutor();
        for (int jobNumber : allExecutors) {
            // ✅ 使用 std::find 简化逻辑
            std::vector<std::string> responsibleModels = userDAO_->getUserResponsibleModels(jobNumber);
            if (std::find(responsibleModels.begin(), responsibleModels.end(), modelName) != responsibleModels.end()) {
                filteredExecutors.push_back(jobNumber);
                // printf("[debug] 执行人 %d 负责模型 %s\n", jobNumber, modelName.c_str());
            }
        }
        retMap["执行人"] = filteredExecutors;
        
        // printf("[info] getOrderRole 模型 %s 筛选结果：审批人 %zu 个，分发人 %zu 个，执行人 %zu 个\n", 
        //        modelName.c_str(), filteredApprovers.size(), filteredDispatchers.size(), filteredExecutors.size());
    }
    
    return retMap;
}
//如果登录验证成功返回的是非空的optional对象
std::optional<User> UserService::login(const std::string& account, const std::string& password) {
    std::optional<User> ret;

    if(userDAO_->login(account,password))
    {
        ret.emplace(getUserByJobNumber(stoi(account)));
    }

    return ret;
}

// 根据token获取权限列表
std::vector<std::string> UserService::getPermissions(const std::string& token) {
    if (token.find("admin") == 0) {
        return { "permission.browse", "permission.create", "permission.edit", "permission.remove" };
    }
    else if (token.find("test") == 0) {
        return { "permission.browse" };
    }
    return {};
}

// 修改指定账号的密码，返回是否成功
bool UserService::editPassword(const std::string& account, const std::string& newPassword) {
    // return dao_.updatePassword(account, newPassword);
    return true;
}
