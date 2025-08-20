#pragma once
#include "IUserService.h"
#include "IUserDAO.h"
#include <memory>
#include <nlohmann/json.hpp>

#include <vector>
#include <string>

class UserService :public IUserService
{
public:
    UserService(std::shared_ptr<IUserDAO> sp);
    
    /**
     * @brief 根据工号获取用户信息(获取当前登录的用户信息)
     * @param jobNumber 工号
     * @return 用户信息
     */
    User getUserByJobNumber(int jobNumber)override;

    /**
     * @brief 获取用户的工单列表
     * @param jobNumber 工号
     * @return 用户负责工单列表
     */
    std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber)override;

    /**
     * @brief 获取工单中的角色（审批人、分发人、执行人）
     * @return 角色与工号的映射
     */
    std::map<std::string,std::vector<int>> getOrderRole() override;
    std::map<std::string, std::vector<std::pair<int, std::string>>> getOrderRole_(const std::string& modelName) override;  // 新方法 筛选负责的模型

    /**
     * @brief 用户登录
     * @param account 用户账号
     * @param password 用户密码
     * @return 登录成功返回用户信息的JSON对象，失败返回std::nullopt
     */
    std::optional<User> login(const std::string& account, const std::string& password);

    /**
     * @brief 根据token获取权限列表
     * @param token 用户token
     * @return 权限字符串列表
     */
    std::vector<std::string> getPermissions(const std::string& token);

    /**
     * @brief 修改指定账号的密码
     * @param account 用户账号
     * @param newPassword 新密码
     * @return 修改成功返回true，失败返回false
     */
    bool editPassword(const std::string& account, const std::string& newPassword);
    
private:
    std::shared_ptr<IUserDAO> userDAO_;

    std::vector<User> users_; // 存储用户列表
};