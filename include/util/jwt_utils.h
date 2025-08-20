/*****************************************************************//**
 * \file   jwt_utils.h
 * \brief  生成和校验JWT token
 * 
 * \author 
 * \date   July 2025
 *********************************************************************/
#pragma once
#include "jwt-cpp/jwt.h"
#include <crow.h>
#include <string>

/**
 * @brief 校验JWT token是否合法
 * @param req crow请求对象
 * @return 校验通过返回true，否则返回false
 */
inline bool checkToken(const crow::request& req) {
    std::string token = req.get_header_value("token");
    if (token.empty()) return false;
    std::string secret = "afd7bad3ba4c66ceb0bc5d9e7729489f850c9364ff840ebd0e4a9af23beb8a84";
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ secret })
            .with_issuer("crowTest");
        verifier.verify(decoded);
        // 可进一步校验过期时间等
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

/**
 * @brief 生成JWT token
 * @param account 用户账号
 * @return 生成的token字符串
 */
inline std::string generateToken(const std::string& account) {
    std::string secret = "afd7bad3ba4c66ceb0bc5d9e7729489f850c9364ff840ebd0e4a9af23beb8a84";
    auto token = jwt::create()
        .set_issuer("crowTest")
        .set_type("JWS")
        .set_payload_claim("account", jwt::claim(account))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{ 24 })
        .sign(jwt::algorithm::hs256{ secret });
    return token;
}

/**
 * @brief 根据JWT token获取用户账号
 * @param req crow请求对象
 * @return 成功返回用户账号，失败返回空字符串
 */
inline std::string getAccountFromToken(const crow::request& req) {
    std::string token = req.get_header_value("token");
    if (token.empty()) return "";
    std::string secret = "afd7bad3ba4c66ceb0bc5d9e7729489f850c9364ff840ebd0e4a9af23beb8a84";
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{ secret })
            .with_issuer("crowTest");
        verifier.verify(decoded);
        
        // 获取account claim
        if (decoded.has_payload_claim("account")) {
            return decoded.get_payload_claim("account").as_string();
        }
        return "";
    }
    catch (const std::exception&) {
        return "";
    }
}