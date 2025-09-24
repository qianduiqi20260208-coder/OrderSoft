// WebSocketManager.h
#pragma once
#include <unordered_map>
#include <mutex>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#endif

#include <crow/websocket.h>

struct Session {
    crow::websocket::connection* conn;
    std::chrono::steady_clock::time_point lastPong;
};

class WebSocketManager {
private:
    static inline std::unordered_map<long, Session> sessions;
    static inline std::mutex mtx;

public:
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

    static void sendToUser(long userId, const std::string& msg) {
        std::lock_guard<std::mutex> lock(mtx);
        if (sessions.count(userId)) {
            sessions[userId].conn->send_text(msg);
        }
    }

    // 定时任务调用：发 ping 并清理超时连接
    static void heartbeatSweep() {
        std::lock_guard<std::mutex> lock(mtx);
            auto now = std::chrono::steady_clock::now();

            std::string timestamp = makeIso8601LocalNoMillis();

            for (auto it = sessions.begin(); it != sessions.end();) {
                std::ostringstream json;
                json << "{"
                    << "\"type\":\"ping\","
                    << "\"data\":{},"
                    << "\"timestamp\":\"" << timestamp << "\""
                    << "}";

                it->second.conn->send_text(json.str());

                if (now - it->second.lastPong > std::chrono::seconds(90)) {
                    it = sessions.erase(it); // 超时移除
                } else {
                    ++it;
                }
            }
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


};
