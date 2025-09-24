// WebSocketController.h
#pragma once
#include "WebSocketManager.h"


#include <crow.h>

class WebSocketController {
public:
    static void init(crow::SimpleApp& app) {

        CROW_ROUTE(app, "/ws")
        .websocket(&app)
        .onopen([](crow::websocket::connection& conn){

        })
        .onmessage([](crow::websocket::connection& conn, const std::string& data, bool){
            //将数据转换成json格式
            auto j = crow::json::load(data);
            // 心跳时间的更新
            if (data.find("pong") != std::string::npos) {
                WebSocketManager::touchPong(j["userId"].i());
            }else if(data.find("ack") != std::string::npos){
                // 客户端确认收到消息 这个if分支是用来标记状态为已读的
                extern std::multimap<int,std::string> notify_messages;
                auto it = notify_messages.find(j["userId"].i());
                if(it != notify_messages.end()){    
                    //删除已确认的消息
                    notify_messages.erase(it);
                }
            }else if(data.find("auth") != std::string::npos){ //第一次建立连接时，客户端会发送认证消息
                //建立连接跟用户id之间的映射
                WebSocketManager::addConnection(j["userId"].i(), &conn);

                auto j = crow::json::load(data);
                //将内存中保存的数据推送给用户
                extern std::multimap<int,std::string> notify_messages;
                auto iter = notify_messages.find(j["userId"].i());
                if(iter != notify_messages.end()){
                    //这种情况是一次性发送多条消息，因为用户登录了。
                    std::string notificaionMsg = "{\"type\":\"todo_notification\",\"msg\":\"你有未完成的待办\"}";
                    WebSocketManager::sendToUser(j["userId"].i(), notificaionMsg);
                }
                std::cout << "User " << j["userId"].i() << " connected\n";
            }
        })
        .onclose([](crow::websocket::connection& conn, const std::string&, uint16_t){//超时之后会自动关闭连接 这个地方就用不到了
            
        });

    }
};
