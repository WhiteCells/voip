#pragma once

#include "../singleton.hpp"
#include "../logger.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>
#include <atomic>

class AccountCheck;

class AccountCheckManager :
    public Singleton<AccountCheckManager>
{
    friend class Singleton<AccountCheckManager>;

    struct AccountRegInfo
    {
        void setAcc(std::shared_ptr<AccountCheck> _acc) { acc = _acc; }
        void setCode(int _code) { code = _code; }
        void setReason(std::string _reason) { reason = _reason; }

        std::shared_ptr<AccountCheck> getAcc() { return acc; }
        int getCode() { return code; }
        std::string getReason() { return reason; }

    private:
        std::shared_ptr<AccountCheck> acc {nullptr};
        int code {200};
        std::string reason {""};
    };

public:
    AccountCheckManager() : m_pushed(false) {}

    ~AccountCheckManager() = default;

    void setRequestRegCnt(std::size_t cnt = 5)
    {
        m_request_reg_cnt = cnt;
    }

    void regAccount(const std::shared_ptr<AccountCheck> acc_check)
    {
        std::unique_lock<std::mutex> lock;
        m_acc_vec.push_back(acc_check);
    }

    void onAccountRegState(std::shared_ptr<AccountCheck> acc_check, const std::string &id, int code, std::string reason)
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

    void clear()
    {
        std::unique_ptr<std::mutex> lock;
        m_acc_vec.clear();
        m_acc_map.clear();
    }

private:
    void tryPushRegState()
    {
        if (m_pushed) {
            return;
        }
        if (m_acc_map.size() < m_request_reg_cnt) {
            LOG_INFO("reg less");
            return;
        }
        LOG_INFO("try push regstate");
        for (const auto &[k, v] : m_acc_map) {
            // AccountsRegState state;
            // state.account_id = k;
            // state.status = (v->getCode() == 200) ? SUCCESSED : FAILED;
            // accounts_reg_state.push_back(state);
            // LOG_INFO("{}", k);
        }
        // voip::pushAccountsRegState(accounts_reg_state);
        m_pushed.store(true);
    }

private:
    std::size_t m_request_reg_cnt;
    std::mutex m_mtx;
    std::unordered_map<std::string, std::shared_ptr<AccountRegInfo>> m_acc_map;
    std::vector<std::shared_ptr<AccountCheck>> m_acc_vec;
    std::atomic<bool> m_pushed;
};
