#pragma once

#include "../thread_pool.h"
#include "../singleton.hpp"
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <mutex>
#include <memory>
#include <algorithm>
#include <atomic>

class EventBus : public Singleton<EventBus>
{
    friend class Singleton<EventBus>;

public:
    struct Subscription
    {
        std::atomic<bool> active {true};
        std::function<void()> unsubscribe;

        void cancel()
        {
            if (active.exchange(false)) {
                unsubscribe();
            }
        }
    };

    using SubscriptionPtr = std::shared_ptr<Subscription>;

    explicit EventBus(size_t threadCount = 4) : m_pool(threadCount) {}
    ~EventBus() = default;

    template <typename Event, typename Func>
    SubscriptionPtr subscribe(Func &&handler)
    {
        auto token = std::make_shared<Subscription>();
        auto idx = std::type_index(typeid(Event));
        size_t id;

        std::lock_guard<std::mutex> lock(m_mtx);
        id = m_next_id[idx]++;

        m_handlers[idx].push_back({id,
                                   [h = std::forward<Func>(handler), token, this](std::shared_ptr<void> raw) {
                                       m_pool.addTask([h, token, raw]() {
                                           if (!token->active.load(std::memory_order_acquire)) {
                                               return;
                                           }
                                           try {
                                               auto eventPtr = static_cast<Event *>(raw.get());
                                               h(*eventPtr);
                                           }
                                           catch (const std::exception &ex) {
                                           }
                                           catch (...) {
                                           }
                                       });
                                   },
                                   token});

        token->unsubscribe = [this, idx, id]() {
            std::lock_guard<std::mutex> lock(m_mtx);
            auto &vec = m_handlers[idx];
            vec.erase(std::remove_if(vec.begin(), vec.end(),
                                     [&](const Handler &h) {
                                         return h.id == id;
                                     }),
                      vec.end());
        };

        return token;
    }

    template <typename Event>
    void publish(const Event &event)
    {
        auto idx = std::type_index(typeid(Event));

        std::vector<Handler> call_list;

        {
            std::lock_guard<std::mutex> lock(m_mtx);
            auto it = m_handlers.find(idx);
            if (it == m_handlers.end()) {
                return;
            }

            for (const auto &h : it->second) {
                if (h.token->active.load(std::memory_order_acquire)) {
                    call_list.push_back(h);
                }
            }
        }

        auto sp = std::make_shared<Event>(event);
        std::shared_ptr<void> raw(sp, sp.get());

        for (const auto &h : call_list) {
            h.func(raw);
        }
    }

private:
    struct Handler
    {
        size_t id;
        std::function<void(std::shared_ptr<void>)> func;
        SubscriptionPtr token;
    };

    mutable std::mutex m_mtx;
    std::unordered_map<std::type_index, std::vector<Handler>> m_handlers;
    std::unordered_map<std::type_index, size_t> m_next_id;

    ThreadPool m_pool;
};
