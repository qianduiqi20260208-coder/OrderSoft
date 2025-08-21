#include "UserService.h"
#include <algorithm>

UserService::UserService(std::shared_ptr<IUserDAO> sp):userDAO_(sp)
{
}

User UserService::getUserByJobNumber(int jobNumber)
{
    User user;
    auto ret = userDAO_->getUser();
    for(auto ele:ret)
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

// 添加模型名参数，所有角色都根据模型筛选
std::map<std::string, std::vector<std::pair<int, std::string>>> UserService::getOrderRole_(const std::string& modelName)
{
    std::map<std::string, std::vector<std::pair<int, std::string>>> retMap;

    if (modelName.empty()) {
        // 如果没有传入模型名，返回所有角色的用户
        retMap["审批人"] = userDAO_->getOrderApprover_();
        retMap["分发人"] = userDAO_->getOrderDispatcher_();
        retMap["执行人"] = userDAO_->getOrderExecutor_();
    } else {
        // 筛选审批人
        std::vector<std::pair<int, std::string>> filteredApprovers;
        auto allApprovers = userDAO_->getOrderApprover_();
        for (const auto& approver : allApprovers) {
            std::vector<std::string> responsibleModels = userDAO_->getUserResponsibleModels(approver.first);
            if (std::find(responsibleModels.begin(), responsibleModels.end(), modelName) != responsibleModels.end()) {
                filteredApprovers.push_back(approver);
            }
        }
        retMap["审批人"] = filteredApprovers;

        // 筛选分发人
        std::vector<std::pair<int, std::string>> filteredDispatchers;
        auto allDispatchers = userDAO_->getOrderDispatcher_();
        for (const auto& dispatcher : allDispatchers) {
            std::vector<std::string> responsibleModels = userDAO_->getUserResponsibleModels(dispatcher.first);
            if (std::find(responsibleModels.begin(), responsibleModels.end(), modelName) != responsibleModels.end()) {
                filteredDispatchers.push_back(dispatcher);
            }
        }
        retMap["分发人"] = filteredDispatchers;

        // 筛选执行人
        std::vector<std::pair<int, std::string>> filteredExecutors;
        auto allExecutors = userDAO_->getOrderExecutor_();
        for (const auto& executor : allExecutors) {
            std::vector<std::string> responsibleModels = userDAO_->getUserResponsibleModels(executor.first);
            if (std::find(responsibleModels.begin(), responsibleModels.end(), modelName) != responsibleModels.end()) {
                filteredExecutors.push_back(executor);
            }
        }
        retMap["执行人"] = filteredExecutors;
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
bool UserService::editPassword(const std::string& userId, const std::string& oldPassword, const std::string& newPassword) {
    return userDAO_->updatePassword(userId, oldPassword, newPassword);
}
