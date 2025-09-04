#ifndef _TYPES_H_
#define _TYPES_H_

#include <string>

enum WsSessionType {
    kVoip,
    kRobot,
};

enum WsSessionStatus {
    kUsed,
    kFree,
    kNone,
};

typedef std::string WsSessionId;

#endif // _TYPES_H_