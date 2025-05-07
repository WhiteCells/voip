#ifndef _CALLPOOL_H_
#define _CALLPOOL_H_

#include "singleton.hpp"
#include "vcall.h"

#include <vector>
#include <mutex>

class CallPool : public Singleton<CallPool>
{
public:
    ~CallPool();

private:
    CallPool();

    std::vector<voip::VCall> m_calls;
    std::mutex m_mtx;
};

#endif // _CALLPOOL_H_