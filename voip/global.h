#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include <pjsua2.hpp>

extern pj::Endpoint endpoint;

void startEndpointLib(unsigned port = 5060);

#endif // _GLOBAL_H_