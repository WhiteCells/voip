#ifndef _ACCOUNT_POOL_H_
#define _ACCOUNT_POOL_H_

#include "singleton.hpp"
#include "vaccount.h"

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

class AccountPool : public Singleton<AccountPool>
{
public:
    using VAccountSPtr = std::shared_ptr<voip::VAccount>;

public:
    ~AccountPool();

    void addAccount();
    VAccountSPtr getAccount();

private:
    AccountPool();

private:
    std::size_t m_pool_size;
    std::queue<VAccountSPtr> m_accounts_que;
    std::condition_variable m_has_acc_condition;
    std::mutex m_has_acc_mtx;
};

#endif // _ACCOUNT_POOL_H_