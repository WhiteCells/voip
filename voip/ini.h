#ifndef _INI_H_
#define _INI_H_

// #include "singleton.hpp"
#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, std::string> cfg_map;

void loadINICfg(const std::string &filename = ".env");

// class INI : public Singleton<INI>
// {
// public:
//     virtual ~INI();
//     const std::string &operator[](const std::string &key) const;
//     INI &operator()()
//     {
//         return *INI::getInstance();
//     }

//     static void init(const std::string &filepath)
//     {
//     }

// private:
//     void load(const std::string &filepath);
//     void trim(const std::string &str);
// };

#endif // _INI_H_
