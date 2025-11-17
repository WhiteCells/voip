#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <mutex>
#include <memory>
#include <algorithm>
#include <atomic>
#include "../thread_pool.h"

class EventBus
{
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

    EventBus(size_t threadCount = 4)
        : pool_(threadCount, false) {}
    ~EventBus() = default;

    template <typename Event, typename Func>
    SubscriptionPtr subscribe(Func &&handler)
    {
        auto token = std::make_shared<Subscription>();
        auto idx = std::type_index(typeid(Event));

        {
            std::lock_guard<std::mutex> lg(mutex_);
            size_t id = next_id_[idx]++;
            auto &handlers = events_[idx];

            handlers.push_back({id,
                                [h = std::forward<Func>(handler), this](std::shared_ptr<const void> p) {
                                    auto pe = std::shared_ptr<const Event>(p, static_cast<const Event *>(p.get()));
                                    pool_.addTask([h, pe]() {
                                        h(*pe);
                                    });
                                },
                                token});

            token->unsubscribe = [this, idx, id]() {
                std::lock_guard<std::mutex> lg(mutex_);
                auto &handlers = events_[idx];
                handlers.erase(std::remove_if(handlers.begin(), handlers.end(), [id](auto &h) {
                                   return h.id == id;
                               }),
                               handlers.end());
            };
        }
        return token;
    }

    template <typename Event>
    void publish(const Event &event)
    {
        std::vector<std::function<void(std::shared_ptr<const void>)>> copy;
        auto idx = std::type_index(typeid(Event));

        {
            std::lock_guard<std::mutex> lg(mutex_);
            auto it = events_.find(idx);
            if (it == events_.end()) {
                return;
            }

            for (auto &h : it->second) {
                if (h.token->active.load()) {
                    copy.push_back(h.func);
                }
            }
        }

        auto sp = std::make_shared<Event>(event);
        std::shared_ptr<const void> p_void(sp, static_cast<const void *>(sp.get()));

        for (auto &fn : copy) {
            fn(p_void);
        }
    }

private:
    struct HandlerBase
    {
        size_t id;
        std::function<void(std::shared_ptr<const void>)> func;
        SubscriptionPtr token;
    };

    std::mutex mutex_;
    std::unordered_map<std::type_index, std::vector<HandlerBase>> events_;
    std::unordered_map<std::type_index, size_t> next_id_;
    ThreadPool pool_;
};
