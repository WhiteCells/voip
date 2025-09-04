#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <string>
#include <unordered_map>

extern std::string backend_host;
extern std::string backend_port;
extern std::unordered_map<std::string, std::string> cfg;

#endif // _GLOBAL_H_