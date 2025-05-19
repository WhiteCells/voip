#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <pjsua2.hpp>
#include <string>

extern pj::Endpoint endpoint;

extern unsigned thread_num;

extern std::string backend_host;

extern std::string backend_port;

void startEndpointLib(unsigned port = 5060);

#endif // _GLOBAL_H_