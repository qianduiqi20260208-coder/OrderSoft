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
    std::string secret = "your_secret_key";
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
    std::string secret = "your_secret_key";
    auto token = jwt::create()
        .set_issuer("crowTest")
        .set_type("JWS")
        .set_payload_claim("account", jwt::claim(account))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{ 24 })
        .sign(jwt::algorithm::hs256{ secret });
    return token;
}