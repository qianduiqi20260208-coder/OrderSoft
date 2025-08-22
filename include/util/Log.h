#include "DBConnectionManager.h"
#include "jwt_utils.h"

template<typename Handler>
auto withAspect(Handler&& handler) {
    return [handler = std::forward<Handler>(handler)](const crow::request& req) {

        // 执行原始处理逻辑
        auto response = handler(req);

        char sql[99999];
        //使用静态类进行资源管理
        static DBConnectionManagerForOperationLog dbcon;
        dbcon.ensureConnected();

        std::string user_id = getAccountFromToken(req);
        snprintf(sql, 99999, "INSERT INTO user_operation_log(user_id,request_params,response_params,operation_url) "

        "VALUES('%s','%s', '%s', '%s');",user_id.c_str(),req.body.c_str(),response.body.c_str(),req.url.c_str());	
	    int ret = mysql_real_query(dbcon.mysql, sql, (unsigned long)strlen(sql));

        if(ret)
        {
            printf("sql:%s",sql);
            printf("[error] function:withAspect 失败原因：%s\n", mysql_error(dbcon.mysql));
        }

        return response;
    };
}

//工单附件下载
template<typename Handler>
auto withAspectTicketDownload(Handler&& handler) {
    return [handler = std::forward<Handler>(handler)](const crow::request& req,int ticketId, const std::string& filename) {

        // 执行原始处理逻辑
        auto response = handler(req,ticketId,filename);

        char sql[1024];
        //使用静态类进行资源管理
        static DBConnectionManagerForOperationLog dbcon;
        dbcon.ensureConnected();

        std::string user_id = getAccountFromToken(req);
        snprintf(sql, 1024, "INSERT INTO user_operation_log(user_id,request_params,response_params,operation_url) "
        "VALUES('%s','%s', '%s', '%s');",user_id.c_str(),req.body.c_str(),response.body.c_str(),req.url.c_str());	
	    int ret = mysql_real_query(dbcon.mysql, sql, (unsigned long)strlen(sql));

        if(ret)
        {
            printf("sql:%s",sql);
            printf("[error] function:withAspectTicketDownload 失败原因：%s\n", mysql_error(dbcon.mysql));
        }

        return response;
    };
}

//获取审批人列表
template<typename Handler>
auto withAspectApproveList(Handler&& handler) {
    return [handler = std::forward<Handler>(handler)](const crow::request& req,const std::string& modelId) {

        // 执行原始处理逻辑
        auto response = handler(req,modelId);

        char sql[1024];
        //使用静态类进行资源管理
        static DBConnectionManagerForOperationLog dbcon;
        dbcon.ensureConnected();

        std::string user_id = getAccountFromToken(req);
        snprintf(sql, 1024, "INSERT INTO user_operation_log(user_id,request_params,response_params,operation_url) "
        "VALUES('%s','%s', '%s', '%s');",user_id.c_str(),req.body.c_str(),response.body.c_str(),req.url.c_str());	
	    int ret = mysql_real_query(dbcon.mysql, sql, (unsigned long)strlen(sql));

        if(ret)
        {
            printf("sql:%s",sql);
            printf("[error] function:withAspect 失败原因：%s\n", mysql_error(dbcon.mysql));
        }

        return response;
    };
}
