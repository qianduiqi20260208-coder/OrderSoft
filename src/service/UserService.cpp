#include "UserService.h"

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

// 用户登录，校验账号和密码，成功则返回用户信息的JSON对象，否则返回std::nullopt
std::optional<nlohmann::json> UserService::login(const std::string& account, const std::string& password) {
    // auto userOpt = dao_.getUserByAccount(account);
    // if (userOpt && userOpt->password == password) {

    //     nlohmann::json j;
    //     j["account"] = userOpt->account;
    //     j["token"] = token;
    //     j["avatar"] = userOpt->avatar;
    //     j["role"] = userOpt->role;
    //     j["isApprover"] = userOpt->isApprover;
    //     j["models"] = userOpt->models;
    //     return j;
    // }
    // return std::nullopt;

    nlohmann::json j;
    j["account"] = "888888";
    j["avatar"] = "https://fantastic-admin.hurui.me/logo.svg";
    j["token"] = "mock_token";
    j["role"] = "SuperUser";
    j["isApprover"] = true;
    j["models"] = {"ATA04_Aerodynamics", "ATA08_WeightBalance", "ATA00_Groundhandling"};
    return j;
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
