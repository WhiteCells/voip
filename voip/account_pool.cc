#include "account_pool.h"
#include "request.hpp"

#include <iostream>

AccountPool::~AccountPool()
{
}

void AccountPool::addAccount()
{
}

AccountPool::VAccountUPtr AccountPool::getAccount()
{
    std::unique_lock<std::mutex> lock(m_has_acc_mtx);
    m_has_acc_condition.wait(lock, [this]() {
        return m_accounts_que.empty();
    });
    return std::move(m_accounts_que.front());
}

void AccountPool::recycleAccount()
{
}

AccountPool::AccountPool()
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
    auto resp = voip::httpRequest("localhost", "50010", "/accounts", voip::http::verb::get);
    for (const auto &accounts : resp["accounts"]) {
        std::cout << accounts["name"].asString()
                  << accounts["password"].asString()
                  << std::endl;
    }
}
