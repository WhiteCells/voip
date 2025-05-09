#include "account_pool.h"
#include "request.hpp"
#include "io_context_pool.h"
#include "vaccount.h"

#include <iostream>

VAccountPool::~VAccountPool()
{
}

void VAccountPool::addVAccount(VAccountUPtr vaccount)
{

}

VAccountPool::VAccountUPtr VAccountPool::getVAccount()
{
    std::unique_lock<std::mutex> lock(m_has_vacc_mtx);
    return std::move(m_vaccounts_que.front());
}

void VAccountPool::recycleVAccount(VAccountUPtr vaccount)
{
}

VAccountPool::VAccountPool()
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
    asio::io_context &ioc = IOContextPool::getInstance()->getIOContext();
    auto resp = voip::httpRequest(ioc, "localhost", "5000", "/accounts/123", voip::http::verb::get);
    for (const auto &accounts : resp["accounts"]) {
        std::cout << accounts["name"].asString()
                  << accounts["password"].asString()
                  << std::endl;
    }
}
