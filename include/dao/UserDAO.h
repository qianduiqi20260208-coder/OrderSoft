#pragma once
#include "IUserDAO.h"
#include <mysql.h>
#define SQL_MAX 1024		// sql语句字符数组最大值


class UserDAO :public IUserDAO
{
public:
    UserDAO(MYSQL* ms);
    std::vector<User> getUser()override;
    //返回一个空指针放在vector里代表查询失败
    std::vector<std::shared_ptr<Ticket>> getUserOrder(int jobNumber)override;
    // 查询指定用户负责的模型
    std::vector<std::string> getUserResponsibleModels(int userId) override;

    //获取工单中的角色
    // 查询审批人
    std::vector<int> getOrderApprover() override;
    std::vector<std::pair<int, std::string>> getOrderApprover_() override; // 查询审批人（工号+姓名）

    // 查询分发人
    std::vector<int> getOrderDispatcher() override;
    std::vector<std::pair<int, std::string>> getOrderDispatcher_() override; // 查询分发人（工号+姓名）

    // 查询执行人
    std::vector<int> getOrderExecutor()  override;
    std::vector<std::pair<int, std::string>> getOrderExecutor_() override; // 查询执行人（工号+姓名）

    bool login(std::string account,std::string password) override;

    bool updatePassword(const std::string& userId, const std::string& oldPassword, const std::string& newPassword) override;

    ~UserDAO();
private:
    //mysql套件
    MYSQL* mysql;
    char sql[SQL_MAX];		// 存储sql语句
    MYSQL_RES* res;
    MYSQL_ROW row;
    int ret;
    std::shared_ptr<Ticket> getUserConcreteOrder(std::string,int);
public:
    static bool downloadAttachment(std::shared_ptr<TicketReproduce>);
    static std::string queryModelVersion(int modelVersionId);

    static std::string queryBaseModelVersion(int modelVersionId);

    static TicketExecutor queryTicketExecutor(int workOrderId);
    static std::string queryProductAuthorization(int);
};

void add_idname_mapping();
