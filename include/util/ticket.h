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
#include <nlohmann/json.hpp>

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
