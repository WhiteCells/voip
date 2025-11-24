#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>

class INIParser
{
public:
    INIParser() = default;

    // 从文件加载，返回是否成功（文件可读）
    bool load(const std::string &path)
    {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs)
            return false;
        // 处理可能的 UTF-8 BOM
        std::string first3;
        char c;
        for (int i = 0; i < 3 && ifs.get(c); ++i)
            first3.push_back(c);
        if (first3.size() == 3 && (unsigned char)first3[0] == 0xEF && (unsigned char)first3[1] == 0xBB && (unsigned char)first3[2] == 0xBF) {
            // BOM 已读，继续
        }
        else {
            // 将读到的字符放回流
            for (int i = (int)first3.size() - 1; i >= 0; --i)
                ifs.unget();
        }
        return parse(ifs);
    }

    // 从字符串加载（便于测试）
    bool loadFromString(const std::string &content)
    {
        std::istringstream iss(content);
        return parse(iss);
    }

    // 基本查询
    bool hasSection(const std::string &section) const
    {
        return data.count(section) > 0;
    }
    bool hasKey(const std::string &section, const std::string &key) const
    {
        auto it = data.find(section);
        if (it == data.end())
            return false;
        return it->second.count(key) > 0;
    }

    // 获取字符串值，若不存在返回 default_val
    std::string getString(const std::string &section, const std::string &key, const std::string &default_val = "") const
    {
        auto it = data.find(section);
        if (it == data.end())
            return default_val;
        auto it2 = it->second.find(key);
        if (it2 == it->second.end())
            return default_val;
        return it2->second;
    }

    // 获取 int/double/bool，若解析失败返回 default_val
    int getInt(const std::string &section, const std::string &key, int default_val = 0) const
    {
        std::string v = getString(section, key, "");
        if (v.empty())
            return default_val;
        try {
            size_t idx;
            long long val = stoll(v, &idx, 0); // 支持 0x... 八进制 0... 等形式
            (void)idx;
            return static_cast<int>(val);
        }
        catch (...) {
            return default_val;
        }
    }
    double getDouble(const std::string &section, const std::string &key, double default_val = 0.0) const
    {
        std::string v = getString(section, key, "");
        if (v.empty())
            return default_val;
        try {
            size_t idx;
            double val = stod(v, &idx);
            (void)idx;
            return val;
        }
        catch (...) {
            return default_val;
        }
    }
    bool getBool(const std::string &section, const std::string &key, bool default_val = false) const
    {
        std::string v = getString(section, key, "");
        if (v.empty())
            return default_val;
        std::string t = toLower(trimCopy(v));
        if (t == "1" || t == "true" || t == "yes" || t == "on")
            return true;
        if (t == "0" || t == "false" || t == "no" || t == "off")
            return false;
        return default_val;
    }

    // 获取所有节名
    std::vector<std::string> sections() const
    {
        std::vector<std::string> res;
        res.reserve(data.size());
        for (auto &p : data) {
            res.push_back(p.first);
        }
        return res;
    }
    // 获取某节内所有键名
    std::vector<std::string> keys(const std::string &section) const
    {
        std::vector<std::string> res;
        auto it = data.find(section);
        if (it == data.end())
            return res;
        res.reserve(it->second.size());
        for (auto &p : it->second)
            res.push_back(p.first);
        return res;
    }

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> data; // section -> (key -> value)

    static inline std::string &trim(std::string &s)
    {
        // in-place trim
        const char *ws = " \t\r\n";
        size_t start = s.find_first_not_of(ws);
        if (start == std::string::npos) {
            s.clear();
            return s;
        }
        size_t end = s.find_last_not_of(ws);
        s = s.substr(start, end - start + 1);
        return s;
    }
    static inline std::string trimCopy(const std::string &s)
    {
        std::string t = s;
        trim(t);
        return t;
    }
    static inline std::string toLower(std::string s)
    {
        for (char &c : s)
            c = static_cast<char>(tolower((unsigned char)c));
        return s;
    }

    // 在不在引号内的情况下找到注释起始位置（';' 或 '#'）
    static inline size_t findCommentPos(const std::string &line)
    {
        bool inSingle = false, inDouble = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (c == '\'' && !inDouble)
                inSingle = !inSingle;
            else if (c == '"' && !inSingle)
                inDouble = !inDouble;
            else if (!inSingle && !inDouble && (c == ';' || c == '#'))
                return i;
            else if (c == '\\' && (inSingle || inDouble)) {
                // 跳过转义字符后的一个字符
                ++i;
            }
        }
        return std::string::npos;
    }

    static inline std::string unquote(const std::string &s)
    {
        if (s.size() >= 2) {
            if ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\'')) {
                // 去除首尾引号并处理简单的转义（\"、\\ 等）
                std::string inner = s.substr(1, s.size() - 2);
                std::string out;
                out.reserve(inner.size());
                for (size_t i = 0; i < inner.size(); ++i) {
                    char c = inner[i];
                    if (c == '\\' && i + 1 < inner.size()) {
                        ++i;
                        char n = inner[i];
                        switch (n) {
                            case 'n':
                                out.push_back('\n');
                                break;
                            case 'r':
                                out.push_back('\r');
                                break;
                            case 't':
                                out.push_back('\t');
                                break;
                            default:
                                out.push_back(n);
                                break;
                        }
                    }
                    else
                        out.push_back(c);
                }
                return out;
            }
        }
        return s;
    }

    bool parse(std::istream &is)
    {
        data.clear();
        std::string line;
        std::string curSection = ""; // 默认节是空字符串
        size_t lineno = 0;
        while (std::getline(is, line)) {
            ++lineno;
            // 移除 Windows 结尾
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            std::string raw = line;
            // 去掉注释（但不要删除引号内的字符）
            size_t cpos = findCommentPos(line);
            if (cpos != std::string::npos)
                line = line.substr(0, cpos);
            trim(line);
            if (line.empty()) {
                continue;
            }
            if (line.front() == '[' && line.back() == ']') {
                std::string sectionName = line.substr(1, line.size() - 2);
                trim(sectionName);
                curSection = sectionName;
                if (!data.count(curSection)) {
                    data.emplace(curSection, std::unordered_map<std::string, std::string>());
                }
            }
            else {
                // 键值对：key = value
                size_t eq = std::string::npos;
                // 允许 '=' 在引号外
                bool inS = false, inD = false;
                for (size_t i = 0; i < line.size(); ++i) {
                    char c = line[i];
                    if (c == '\'' && !inD)
                        inS = !inS;
                    else if (c == '\"' && !inS)
                        inD = !inD;
                    else if (c == '=' && !inS && !inD) {
                        eq = i;
                        break;
                    }
                    else if (c == '\\' && (inS || inD))
                        ++i; // skip escaped char
                }
                if (eq == std::string::npos) {
                    // 没有等号，当作标记键存在，值为空
                    std::string key = trimCopy(line);
                    if (!key.empty())
                        data[curSection][key] = std::string();
                }
                else {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    trim(key);
                    trim(val);
                    // 去除行首行尾的引号并处理转义
                    val = unquote(val);
                    data[curSection][key] = val;
                }
            }
        }
        return true;
    }
};
