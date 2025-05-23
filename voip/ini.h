#ifndef _ENV_H_
#define _ENV_H_

#include <string>
#include <unordered_map>

typedef std::unordered_map<std::string, std::string> cfg_map;

void loadINICfg(const std::string &filename = ".env");

#endif // _ENV_H_