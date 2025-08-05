#ifndef _ACCOUNT_CHECK_MANAGER_H_
#define _ACCOUNT_CHECK_MANAGER_H_

#include "singleton.hpp"
#include <memory>
#include <unordered_map>
#include <string>
#include <mutex>
#include <vector>

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
    explicit AccountCheckManager();
    ~AccountCheckManager();

    void setRequestRegCnt(std::size_t cnt = 5);
    void regAccount(const std::shared_ptr<AccountCheck> acc_check);
    void onAccountRegState(std::shared_ptr<AccountCheck> acc_check, const std::string &id, int code, std::string reason);
    void clear();

private:
    void tryPushPushRegState();

private:
    std::size_t m_request_reg_cnt;
    std::mutex m_mtx;
    std::unordered_map<std::string, std::shared_ptr<AccountRegInfo>> m_acc_map;
    std::vector<std::shared_ptr<AccountCheck>> m_acc_vec;
    bool m_pushed = false;
};

#endif // _ACCOUNT_CHECK_MANAGER_H_