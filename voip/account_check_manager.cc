#include "account_check_manager.h"
#include "account_check.h"
#include "request.hpp"

AccountCheckManager::AccountCheckManager(std::size_t cnt) :
    m_cnt(cnt)
{
}

AccountCheckManager::~AccountCheckManager()
{
}

void AccountCheckManager::regAccount(const std::shared_ptr<AccountCheck> acc_check)
{
    std::unique_lock<std::mutex> lock;
    m_acc_vec.push_back(acc_check);
}

void AccountCheckManager::onAccountRegState(std::shared_ptr<AccountCheck> acc_check,
                                            const std::string &id, int code, std::string reason)
{
    std::unique_lock<std::mutex> lock;
    auto info = std::make_shared<AccountRegInfo>();
    info->setAcc(acc_check);
    info->setCode(code);
    info->setReason(reason);
    m_acc_map[id] = info;
    tryPushPushRegState();
}

void AccountCheckManager::clear()
{
    std::unique_ptr<std::mutex> lock;
    m_acc_vec.clear();
    m_acc_map.clear();
}

void AccountCheckManager::tryPushPushRegState()
{
    if (m_pushed || m_acc_map.size() < m_cnt) {
        // todo
        // return;
    }
    // push
    LOG_INFO("try push regstate");
    for (const auto &[k, v] : m_acc_map) {
        LOG_INFO("{}", k);
    }
    m_pushed = true;
}
