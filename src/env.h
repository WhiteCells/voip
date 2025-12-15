#pragma once

#include <string>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <climits>

class Env
{
public:
    static Env &instance()
    {
        static Env inst;
        return inst;
    }

    Env(const Env &) = delete;
    Env &operator=(const Env &) = delete;

    bool load(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            line = trim(line);

            if (line.empty() || line[0] == '#') {
                continue;
            }

            size_t pos = line.find('=');
            if (pos == std::string::npos) {
                continue;
            }

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            // 去掉引号
            if (!value.empty() &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2);
            }

            m_env_map[key] = value;
        }
        file.close();
        return true;
    }

    std::string getStr(const std::string &key, const std::string &default_value = "") const
    {
        auto it = m_env_map.find(key);
        if (it == m_env_map.end()) {
            return default_value;
        }
        return it->second;
    }

    int getInt(const std::string &key, int default_value = INT_MAX) const
    {
        try {
            return std::stoi(getStr(key));
        }
        catch (const std::invalid_argument &) {
            return default_value;
        }
    }

    double getDouble(const std::string &key, double default_value = INT_MAX) const
    {
        try {
            return std::stod(getStr(key));
        }
        catch (const std::invalid_argument &) {
            return default_value;
        }
    }

    float getFloat(const std::string &key, float default_value = INT_MAX) const
    {
        try {
            return std::stof(getStr(key));
        }
        catch (const std::invalid_argument &) {
            return default_value;
        }
    }

    bool getBool(const std::string &key, bool default_value = false) const
    {
        try {
            return toBool(getStr(key));
        }
        catch (const std::invalid_argument &) {
            return default_value;
        }
    }

private:
    Env() = default;

    std::unordered_map<std::string, std::string> m_env_map;

    static std::string trim(const std::string &s)
    {
        size_t start = 0;
        while (start < s.size() && std::isspace((unsigned char)s[start])) {
            start++;
        }
        size_t end = s.size();
        while (end > start && std::isspace((unsigned char)s[end - 1])) {
            end--;
        }
        return s.substr(start, end - start);
    }

    static bool toBool(const std::string &v)
    {
        std::string t = v;
        std::transform(t.begin(), t.end(), t.begin(), ::tolower);

        if (t == "1" || t == "true" || t == "True" || t == "TRUE") {
            return true;
        }
        if (t == "0" || t == "false" || t == "False" || t == "FALSE") {
            return false;
        }

        throw std::invalid_argument("Invalid boolean format: " + v);
    }
};
