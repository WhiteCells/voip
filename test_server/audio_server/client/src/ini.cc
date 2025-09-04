#include "ini.h"
#include "global.h"
#include "logger.h"
#include <string>
#include <fstream>

/**
 * @brief 去除多余空格
 *
 * @param str 输入字符串
 * @return std::string
 */
std::string trimSpace(const std::string &str)
{
    std::size_t start = 0, end = str.size() - 1;
    while (start < str.size() && std::isspace(str[start])) {
        ++start;
    }
    if (start == str.size()) {
        return "";
    }
    while (end > start && std::isspace(str[end])) {
        --end;
    }
    return str.substr(start, end - start + 1);
}

/**
 * @brief 加载 .ini 配置文件
 *
 * @param filename .ini 配置文件路径
 */
void loadINICfg(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        LOG_CRITICAL("load ini file failed");
        throw std::runtime_error {"load ini file failed"};
    }
    std::string line;

    while (std::getline(file, line)) {
        line = trimSpace(line);
        if (line.empty()) {
            continue;
        }
        if (line[0] == '#') {
            continue;
        }
        std::size_t eq_pos = line.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line.substr(0, eq_pos);
            std::string val = line.substr(eq_pos + 1);
            key = trimSpace(key);
            val = trimSpace(val);
            cfg[key] = val;
            // std::cout << "key: " << key << ", "
            //           << "val: " << val << "\n";
        }
    }

    backend_host = cfg["BACKEND_HOST"];
    backend_port = cfg["BACKEND_PORT"];
}