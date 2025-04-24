#include "ini.h"

#include <string>
#include <fstream>

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

voip::cfg_map voip::loadINICfg(const std::string &filename)
{
    std::ifstream file(filename);
    std::string line;
    voip::cfg_map cfg;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        line = trimSpace(line);
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

    return cfg;
}

#ifdef TEST_INI

int main(int argc, char *argv[])
{
    auto cfg_map = loadEnv(".env");
    for (auto &p : cfg_map) {
        std::cout << p.first << ", "
                  << p.second << "\n";
    }
    return 0;
}

#endif