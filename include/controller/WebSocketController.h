// WebSocketController.h
#pragma once
#include "WebSocketManager.h"


#include <crow.h>

class WebSocketController {
public:
    static void init(crow::App<crow::CORSHandler>& app) {

        CROW_ROUTE(app, "/ws")
        .websocket(&app)
        .onopen([](crow::websocket::connection& conn){
            std::cout << "New WebSocket connection established\n";
        })
        .onmessage([](crow::websocket::connection& conn, const std::string& data, bool){
            //将数据转换成json格式
            auto j = crow::json::load(data);
            // 心跳时间的更新
            if (data.find("pong") != std::string::npos) {
                WebSocketManager::touchPong(j["userId"].i());
                std::cout<<"收到pong user:"<<j["userId"].i()<<std::endl;
            }else if(data.find("ack") != std::string::npos){
                // 客户端确认收到消息 这个if分支是用来标记状态为已读的
                WebSocketManager::markNotificationAsRead(j["id"].i());
                std::cout<<"收到ack"<<std::endl;
            }else if(data.find("auth") != std::string::npos){ //第一次建立连接时，客户端会发送认证消息
                //建立连接跟用户id之间的映射
                WebSocketManager::addConnection(j["userId"].i(), &conn);
                std::cout << "User " << j["userId"].i() << " connected\n";

                //将未读消息发送给用户
                auto msgs = WebSocketManager::sendNotificationWhenConnected(j["userId"].i());
                for(auto& msg : msgs){
                    std::cout<<"发送给用户:"<<j["userId"].i()<<std::endl;
                    conn.send_text(msg);
                }
            
            }
        })
        .onclose([](crow::websocket::connection& conn, const std::string&, uint16_t){//超时之后会自动关闭连接 这个地方就用不到了
            std::cout << "WebSocket connection disconnected\n";
            
            WebSocketManager::eraseDisconnection(&conn);
            
        });

    }
};
