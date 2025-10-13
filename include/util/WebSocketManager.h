// WebSocketManager.h
#pragma once
#include <unordered_map>
#include <mutex>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <crow/websocket.h>
#include "DBConnectionManager.h"
#include <sstream>
#include <crow/json.h>
#include "TicketDAO.h"

struct Session {
    crow::websocket::connection* conn;
    std::chrono::steady_clock::time_point lastPong;
};

class WebSocketManager {
private:
    static inline std::unordered_map<long, Session> sessions;
    static inline std::mutex mtx;

public:
    //清除断开连接的连接
    static void eraseDisconnection(crow::websocket::connection* conn)
    {
        //用户主动断开连接的情况
        std::lock_guard<std::mutex> lock(mtx);
        for (auto it = sessions.begin(); it != sessions.end();) {
            if (it->second.conn == conn) {  // 指针匹配
                it = sessions.erase(it);
            } else {
                ++it;
            }
        }
    }

    static void addConnection(long userId, crow::websocket::connection* conn) {
        std::lock_guard<std::mutex> lock(mtx);
        sessions[userId] = {conn, std::chrono::steady_clock::now()};
    }

    static void removeConnection(long userId) {
        std::lock_guard<std::mutex> lock(mtx);
        sessions.erase(userId);
    }

    static void touchPong(long userId) {
        std::lock_guard<std::mutex> lock(mtx);
        if (sessions.count(userId)) {
            sessions[userId].lastPong = std::chrono::steady_clock::now();
        }
    }

    static void sendToUserAndSaveMsg(std::vector<long> userIds, crow::json::wvalue& msg) {
        std::lock_guard<std::mutex> lock(mtx);

        //先持久化存储，这样才会有messageId数据
        int messageId = saveMsg(msg);

        for(auto userId : userIds) {
            if (sessions.count(userId)) {
                msg["messageId"] = std::to_string(messageId);
                std::cout<<"给"<<userId<<"发"<<msg.dump()<<std::endl;
                //给指定的所有用户发消息
                sessions[userId].conn->send_text(msg.dump());
            }
        }

        std::cout<<"send msg:"<<msg.dump()<<std::endl;

    }



    // 定时任务调用：发 ping 并清理超时连接
    static void heartbeatSweep(TicketDAO& ticketDAO) {
        std::lock_guard<std::mutex> lock(mtx);
            auto now = std::chrono::steady_clock::now();

            std::string timestamp = makeIso8601LocalNoMillis();

            for (auto it = sessions.begin(); it != sessions.end();) {
                long userId = it->first;
                // 调用getUserPendingWorkOrdersCount获取待处理工单数量
                int pendingCount = ticketDAO.getUserPendingWorkOrdersCount(std::to_string(userId));
                std::ostringstream json;
                json << "{"
                    << "\"type\":\"ping\","
                    << "\"data\":{\"pendingCount\":" << pendingCount << "},"
                    << "\"timestamp\":\"" << timestamp << "\""
                    << "}";
                if(it->second.conn)
                {
                    //这个异常只需放行就可以了
                    try
                    {
                        it->second.conn->send_text(json.str());
                    }
                    catch (...) {
                        std::cerr << "send_text 发生未知异常" << std::endl;
                    }
                }

                std::cout<<"ping:"<<json.str()<<std::endl;
                if (now - it->second.lastPong > std::chrono::seconds(90)) {
                    it = sessions.erase(it); // 超时移除
                } else {
                    ++it;
                }
            }
    }

    // 将消息标记为已读
    static void markNotificationAsRead(int notificationId)
    {
        auto connGuard = DBConnectionManager::getPoolConnection();
        MYSQL* conn = connGuard.get();
        if (!conn) {
            std::cerr << "WebSocketManager::notificationHasRead 无法获取数据库连接" << std::endl;
            return;
        }

        std::stringstream ss;
        ss << "UPDATE notification SET status='已读' WHERE id=" << notificationId;
        std::string query = ss.str();

        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::notificationHasRead 更新消息状态失败: " << mysql_error(conn) << std::endl;
        } else {
            std::cout << "WebSocketManager::notificationHasRead 消息状态更新成功" << std::endl;
        }
    }
    
    // 用户上线时的数据发送
    static std::vector<std::string> sendNotificationWhenConnected(int userId)
    {
        auto connGuard = DBConnectionManager::getPoolConnection();
        MYSQL* conn = connGuard.get();
        if (!conn) {
            std::cerr << "WebSocketManager::notificationHasRead 无法获取数据库连接" << std::endl;
            return {};
        }

        std::stringstream ss;
        ss << "SELECT message FROM notification WHERE status='未读' AND receiver_id=" << userId << " ORDER BY created_at DESC";
        std::string query = ss.str();
        std::vector<std::string> messages;
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::sendNotificationWhenConnected 查询未读消息失败: " << mysql_error(conn) << std::endl;
            return messages;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                if (row[0]) {
                    messages.push_back(row[0]);
                }
            }
            mysql_free_result(res);
        } else {
            std::cerr << "WebSocketManager::sendNotificationWhenConnected 获取结果集失败: " << mysql_error(conn) << std::endl;
        }
        return messages;
    }

    static std::string queryNameById(int userId)
    {
        auto connGuard = DBConnectionManager::getPoolConnection();
        MYSQL* conn = connGuard.get();
        if (!conn) {
            std::cerr << "WebSocketManager::queryNameById 无法获取数据库连接" << std::endl;
            return "";
        }

        std::stringstream ss;
        ss << "SELECT real_name FROM user WHERE username = " << userId << ";";
        std::string query = ss.str();
        std::string name;
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::queryNameById 查询未读消息失败: " << mysql_error(conn) << std::endl;
            return "";
        }
        MYSQL_RES* res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                if (row[0]) {
                    name = row[0];
                }
            }
            mysql_free_result(res);
        } else {
            std::cerr << "WebSocketManager::queryNameById 获取结果集失败: " << mysql_error(conn) << std::endl;
        }
        return name;
    }

    static std::string queryClientByWorkOrderId(int workOrderId)
    {
        auto connGuard = DBConnectionManager::getPoolConnection();
        MYSQL* conn = connGuard.get();
        if (!conn) {
            std::cerr << "WebSocketManager::queryClientByWorkOrderId 无法获取数据库连接" << std::endl;
            return "";
        }

        std::stringstream ss;

        //查询交付发送工单表
        ss << "SELECT target_customer FROM delivery_send WHERE work_order_id = " << workOrderId << ";";
        std::string query = ss.str();
        std::string name;
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::queryNameById 查询未读消息失败: " << mysql_error(conn) << std::endl;
            return "";
        }
        MYSQL_RES* res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            if ((row = mysql_fetch_row(res))) {
                if (row[0]) {
                    name = row[0];
                }
            }
            mysql_free_result(res);
        } else {
            std::cerr << "WebSocketManager::queryNameById 获取结果集失败: " << mysql_error(conn) << std::endl;
        }

        //查询封装发送工单表
        ss.clear();
        ss.str("");
        ss << "SELECT target_customer FROM package_send WHERE work_order_id = " << workOrderId << ";";
        query = ss.str();
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::queryNameById 查询未读消息失败: " << mysql_error(conn) << std::endl;
            return "";
        }
        res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            if ((row = mysql_fetch_row(res))) {
                if (row[0]) {
                    name = row[0];
                }
            }
            mysql_free_result(res);
        } else {
            std::cerr << "WebSocketManager::queryNameById 获取结果集失败: " << mysql_error(conn) << std::endl;
        }

        return name;
    }

    static Ticket queryWorkOrderInfo(int orderId)
    {
        auto connGuard = DBConnectionManager::getPoolConnection();
        MYSQL* conn = connGuard.get();
        if (!conn) {
            std::cerr << "WebSocketManager::notificationHasRead 无法获取数据库连接" << std::endl;
            return Ticket{};
        }
        Ticket ticket;
        std::stringstream ss;
        ss<<"SELECT created_at,type,model.model_name,status_todo,creator_id,approver_id,dispatcher_id FROM work_order inner join model on model.ata_code = work_order.model WHERE work_order.id = "<<orderId<<"";
        std::string query = ss.str();
        
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::queryWorkOrderInfo: " << mysql_error(conn) << std::endl;
            return ticket;
        }
        MYSQL_RES* res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            if ((row = mysql_fetch_row(res))) {
                if (row[0]) {
                    ticket.createTime = row[0];
                }
                if(row[1])
                {
                    ticket.ticketType = row[1];
                }
                if(row[2])
                {
                    ticket.model = row[2];
                }
                if(row[3])
                {
                    ticket.status = row[3];
                }
                if(row[4])
                {
                    ticket.creatorId = row[4];
                }
                if(row[5])
                {
                    ticket.approverId = row[5];
                }
                if(row[6])
                {
                    ticket.distributorId = row[6];
                }

            }
            mysql_free_result(res);
        } else {
            std::cerr << "WebSocketManager::queryWorkOrderInfo 获取结果集失败: " << mysql_error(conn) << std::endl;
        }

        return ticket;
    }


private:


    static std::string makeIso8601LocalNoMillis() {
        // 当前本地时间
        auto now_sys = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now_sys);

        std::ostringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
        return ss.str();
    }
    //返回插入数据的id
    static int saveMsg(crow::json::wvalue& jsonObj)
    {
        int messageId = -1;
        //将消息存储在数据库中
        auto connGuard = DBConnectionManager::getPoolConnection();
        MYSQL* conn = connGuard.get();
        if (!conn) {
            std::cerr << "WebSocketManager::saveMsg 无法获取数据库连接" << std::endl;
            return messageId;
        }

        //获取需要插入的数据
        std::string senderId = jsonObj["userId"].dump();
        std::string receiverId = jsonObj["data"]["userId"].dump();

        //执行插入操作
        std::stringstream ss;
        ss<<"INSERT INTO notification(message,status,created_at,receiver_id) VALUES("

          << "'" << jsonObj.dump() << "',"
          << "'未读',"
          << "NOW(),"
          <<receiverId
          << ")";
        std::string query = ss.str();   
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::saveMsg 插入消息失败: " << mysql_error(conn) << std::endl;
        } else {
            std::cout << "WebSocketManager::saveMsg 消息插入成功" << std::endl;
        }
        messageId = mysql_insert_id(conn);

        //将消息id也插入到json里
        jsonObj["messageId"] = crow::json::wvalue(std::to_string((int)mysql_insert_id(conn)));
        ss.clear();
        ss.str("");
        LOG_DEBUG("jsonObj:",jsonObj["messageId"] );
        ss << "UPDATE notification SET message='" << jsonObj.dump() << "' WHERE id=" << messageId<<";";
        query = ss.str();
        if (mysql_query(conn, query.c_str())) {
            std::cerr << "WebSocketManager::saveMsg 更新消息ID失败: " << mysql_error(conn) << std::endl;
        } else {
            std::cout << "WebSocketManager::saveMsg 消息ID更新成功" << std::endl;
        }

        return messageId;

    }



};
