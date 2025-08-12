#include <vector>
#include "account_check_manager.h"
#include "account_check.h"
#include "request.hpp"
#include "global.h"

AccountCheckManager::AccountCheckManager() :
    m_pushed(false)
{
}

AccountCheckManager::~AccountCheckManager()
{
}

void AccountCheckManager::setRequestRegCnt(std::size_t cnt)
{
    m_request_reg_cnt = cnt;
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
    LOG_INFO("m_acc_map size: {}", m_acc_map.size());
    tryPushRegState();
}

void AccountCheckManager::clear()
{
    std::unique_ptr<std::mutex> lock;
    m_acc_vec.clear();
    m_acc_map.clear();
}

void AccountCheckManager::tryPushRegState()
{
    if (m_pushed) {
        return;
    }
    if (m_acc_map.size() < m_request_reg_cnt) {
        LOG_INFO("reg less");
        return;
    }
    std::vector<AccountsRegState> accounts_reg_state;
    // push
    LOG_INFO("try push regstate");
    for (const auto &[k, v] : m_acc_map) {
        AccountsRegState state;
        state.account_id = k;
        state.status = (v->getCode() == 200) ? SUCCESSED : FAILED;
        accounts_reg_state.push_back(state);
        LOG_INFO("{}", k);
    }
    voip::pushAccountsRegState(accounts_reg_state);
    m_pushed.store(true);
}
