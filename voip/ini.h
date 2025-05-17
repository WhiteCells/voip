#ifndef _ENV_H_
#define _ENV_H_

#include <string>
#include <unordered_map>

namespace voip {

typedef std::unordered_map<std::string, std::string> cfg_map;

void loadINICfg(const std::string &filename);

extern cfg_map cfg;

} // namespace voip

#endif // _ENV_H_