/*****************************************************************//**
 * \file   ticket.h
 * \brief  工单模块相关类型定义以及工具函数
 * 
 * \author 
 * \date   July 2025
 *********************************************************************/
#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <sstream>
#include <nlohmann/json.hpp>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

/**
 * @struct MultipartResult
 * @brief multipart/form-data 解析结果结构体,专用于文件附件
 */
struct MultipartResult {
    std::unordered_map<std::string, std::vector<std::string>> fields;
    std::vector<std::string> saved_files;
};

/**
 * @brief 安全获取 multipart/form-data 字段值
 * @param result 解析结果
 * @param key 字段名
 * @return 字段的第一个值，若不存在则返回空字符串
 */
inline std::string getField(const MultipartResult& result, const std::string& key) {
    auto it = result.fields.find(key);
    if (it != result.fields.end() && !it->second.empty()) {
        return it->second[0];
    }
    return "";
}

inline std::string base64_encode(const std::string& input) {
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.c_str(), input.length());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string encoded(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    return encoded;
}

/**
 * @brief 生成唯一文件名，避免同名文件冲突
 * @param originalFileName 原始文件名（包含扩展名）
 * @return 生成的唯一文件名，格式为：原名_YYYYMMDD_HHMMSS_随机数.扩展名
 * @details 使用年月日时分秒和4位随机数组合确保文件名唯一性，
 *          保留原始文件名的可读性，支持有无扩展名的文件
 * @example 输入 "report.pdf" 可能输出 "report_20250805_143052.pdf"
 */
inline std::string generateUniqueFileName(const std::string& originalFileName) {
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    // 分离文件名和扩展名
    size_t dotPos = originalFileName.find_last_of('.');
    std::string baseName, extension;
    
    if (dotPos != std::string::npos) {
        baseName = originalFileName.substr(0, dotPos);
        extension = originalFileName.substr(dotPos); // 包含点号
    } else {
        baseName = originalFileName;
        extension = "";
    }

    // 构建唯一文件名：原名_YYYYMMDD_HHMMSS.扩展名
    std::ostringstream oss;
    oss << baseName << "_" 
        << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900)  // 年份
        << std::setfill('0') << std::setw(2) << (tm.tm_mon + 1)      // 月份
        << std::setfill('0') << std::setw(2) << tm.tm_mday           // 日
        << "_"
        << std::setfill('0') << std::setw(2) << tm.tm_hour           // 小时
        << std::setfill('0') << std::setw(2) << tm.tm_min            // 分钟
        << std::setfill('0') << std::setw(2) << tm.tm_sec            // 秒
        << extension;
    
    return oss.str();
}
