#pragma once

#include "../singleton.hpp"
#include "outcoming_acc.h"

class OutcomingAccMgr : public Singleton<OutcomingAccMgr>
{
    friend class Singleton<OutcomingAccMgr>;

    struct AccountRegInfo
    {
        void setAcc(std::shared_ptr<OutcomingAcc> _acc) { acc = _acc; }
        void setCode(int _code) { code = _code; }
        void setReason(std::string _reason) { reason = _reason; }

        std::shared_ptr<OutcomingAcc> getAcc() { return acc; }
        int getCode() { return code; }
        std::string getReason() { return reason; }

    private:
        std::shared_ptr<OutcomingAcc> acc {nullptr};
        int code {200};
        std::string reason {""};
    };

public:
    OutcomingAccMgr() : m_pushed(false)
    {
    }

    ~OutcomingAccMgr() = default;

    void setRequestRegCnt(std::size_t cnt = 5)
    {
        m_request_reg_cnt = cnt;
    }

    void regAccount(const std::shared_ptr<OutcomingAcc> acc)
    {
        std::unique_lock<std::mutex> lock;
        m_acc_vec.push_back(acc);
    }

    void onAccountRegState(std::shared_ptr<OutcomingAcc> acc, const std::string &id, int code, std::string reason)
    {
        std::unique_lock<std::mutex> lock;
        auto info = std::make_shared<AccountRegInfo>();
        info->setAcc(acc);
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
    void tryPushRegState();

private:
    std::size_t m_request_reg_cnt;
    std::mutex m_mtx;
    std::unordered_map<std::string, std::shared_ptr<AccountRegInfo>> m_acc_map;
    std::vector<std::shared_ptr<OutcomingAcc>> m_acc_vec;
    std::atomic<bool> m_pushed;
};