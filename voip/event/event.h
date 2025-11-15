#ifndef _EVENT_H_
#define _EVENT_H_
#pragma once

#include <vector>
#include <unordered_map>
#include <mutex>
#include <typeindex>
#include <functional>

class EventBus
{
public:
    EventBus() = default;
    ~EventBus() = default;

    template <typename Event>
    void subscribe(std::function<void(const Event &)> handler)
    {
    }

    template <typename Event>
    void publish(const Event &event)
    {
        // std::lock_guard<std::mutex> lock(m_mtx);
        // auto it =
    }

private:
    // using event_handler_vec = std::vector<std::function()
    // std::mutex m_mtx;
};

#endif // _EVENT_H_