#ifndef _ACCOUNT_POOL_H_
#define _ACCOUNT_POOL_H_

#include "singleton.hpp"
// #include "vaccount.h"

#include <queue>
#include <memory>
#include <mutex>

namespace voip {
class VAccount;
}

class VAccountPool : public Singleton<VAccountPool>
{
    friend class Singleton<VAccountPool>;

public:
    using VAccountUPtr = std::unique_ptr<voip::VAccount>;

public:
    ~VAccountPool();

    void addVAccount(VAccountUPtr vaccount);
    VAccountUPtr getVAccount();
    void recycleVAccount(VAccountUPtr vaccount);

private:
    VAccountPool();

private:
    std::size_t m_pool_size;
    std::queue<VAccountUPtr> m_vaccounts_que;
    std::mutex m_has_vacc_mtx;
};

#endif // _ACCOUNT_POOL_H_