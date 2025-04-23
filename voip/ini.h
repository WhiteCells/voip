#ifndef _ENV_H_
#define _ENV_H_

#include <string>
#include <unordered_map>

namespace voip {

typedef std::unordered_map<std::string, std::string> cfg_map;

cfg_map loadINICfg(const std::string &filename);

} // namespace voip

#endif // _ENV_H_