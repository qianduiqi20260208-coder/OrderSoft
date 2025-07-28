#pragma once
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <algorithm>

class IniReader {
    using Section = std::unordered_map<std::string, std::string>;
    std::unordered_map<std::string, Section> data;

    static std::string trim(const std::string& s) {
        const char* ws = " \t\r\n";
        size_t start = s.find_first_not_of(ws);
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(ws);
        return s.substr(start, end - start + 1);
    }

public:
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) return false;

        std::string line, currentSection;
        while (std::getline(file, line)) {
            // 移除注释
            size_t commentPos = line.find_first_of(";#");
            if (commentPos != std::string::npos)
                line = line.substr(0, commentPos);

            line = trim(line);
            if (line.empty()) continue;

            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.size() - 2);
            } else {
                size_t eqPos = line.find('=');
                if (eqPos == std::string::npos) continue;

                std::string key = trim(line.substr(0, eqPos));
                std::string val = trim(line.substr(eqPos + 1));
                data[currentSection][key] = val;
            }
        }

        return true;
    }

    std::string getString(const std::string& section, const std::string& key, const std::string& def = "") const {
        auto secIt = data.find(section);
        if (secIt == data.end()) return def;
        auto keyIt = secIt->second.find(key);
        if (keyIt == secIt->second.end()) return def;
        return keyIt->second;
    }

    int getInt(const std::string& section, const std::string& key, int def = 0) const {
        try {
            return std::stoi(getString(section, key));
        } catch (...) {
            return def;
        }
    }

    bool getBool(const std::string& section, const std::string& key, bool def = false) const {
        std::string val = getString(section, key);
        std::transform(val.begin(), val.end(), val.begin(), ::tolower);
        if (val == "true" || val == "1") return true;
        if (val == "false" || val == "0") return false;
        return def;
    }
};
