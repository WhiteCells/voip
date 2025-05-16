#include "account_queue.h"
#include "request.hpp"
#include "io_context_pool.h"
#include "vaccount.h"

#include <iostream>

AccountQueue::~AccountQueue()
{
}

void AccountQueue::addVAccount(VAccountUPtr vaccount)
{
    std::unique_lock<std::mutex> lock {m_has_vacc_mtx};
    m_vaccounts_que.push(std::move(vaccount));
}

AccountQueue::VAccountUPtr AccountQueue::getVAccount()
{
    std::unique_lock<std::mutex> lock {m_has_vacc_mtx};
    if (m_vaccounts_que.empty()) {
        return nullptr;
    }
    auto vaccount = std::move(m_vaccounts_que.front());
    m_vaccounts_que.pop();
    return vaccount;
}

void AccountQueue::recycleVAccount(VAccountUPtr vaccount)
{
    std::unique_lock<std::mutex> lock {m_has_vacc_mtx};
    m_vaccounts_que.push(std::move(vaccount));
}

AccountQueue::AccountQueue()
{
    // Request
    /*
        {
            "accounts": ]
                {"": ""},
                {"": ""},
                {"": ""},
            ]
        }
   */
    auto resp = voip::httpRequest( "localhost", "5000", "/accounts/123", voip::http::verb::get);
    for (const auto &accounts : resp["accounts"]) {
        std::cout << accounts["name"].asString()
                  << accounts["password"].asString()
                  << std::endl;
    }
}
