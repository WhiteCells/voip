#include "account_pool.h"

AccountPool::~AccountPool()
{
}

void AccountPool::addAccount()
{
}

AccountPool::VAccountSPtr AccountPool::getAccount()
{
    std::unique_lock<std::mutex> lock(m_has_acc_mtx);
    m_has_acc_condition.wait(lock, [this]() {
        return m_accounts_que.empty();
    });
    return m_accounts_que.front();
}

AccountPool::AccountPool() :
    m_pool_size(10),
    m_has_acc_mtx()
{
    for (std::size_t i = 0; i < m_pool_size; ++i) {
        m_accounts_que.push(std::make_shared<voip::VAccount>());
    }
}
