#ifndef _COORDINATOR_H_
#define _COORDINATOR_H_

#include "logger.h"

#include <pjsua2.hpp>
#include <vector>
#include <mutex>
// #include <iostream>

class Coordinator
{
public:
    Coordinator() :
        m_connected(false)
    {
    }

    ~Coordinator()
    {
        for (auto caller : m_callers) {
            caller->hangup(m_hangup_param);
        }
    }

    void registerCall(pj::Call *caller)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_connected) {
            return;
        }
        m_callers.push_back(caller);
    }

    void clearCall()
    {
        m_callers.clear();
    }

    void onCallConnected(pj::Call *connected_caller)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_connected) {
            return;
        }
        m_connected = true;
        for (auto *caller : m_callers) {
            if (caller != connected_caller) {
                try {
                    caller->hangup(m_hangup_param);
                }
                catch (...) {
                    LOG_ERROR("failed to hangup"); // todo
                }
            }
        }
    }

private:
    bool m_connected;
    std::mutex m_mtx;
    std::vector<pj::Call *> m_callers;
    pj::CallOpParam m_hangup_param;
};

#endif // _COORDINATOR_H_