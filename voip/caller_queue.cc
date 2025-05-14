#include "caller_queue.h"

#include <pjsua2.hpp>

CallerQueue::CallerQueue()
{
}

CallerQueue::~CallerQueue()
{
}

void CallerQueue::addCaller(CallerUPtr vcall)
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que.push(std::move(vcall));
    m_que_cond.notify_one();
}

CallerQueue::CallerUPtr CallerQueue::getCaller()
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que_cond.wait(lock, [this]() {
        return !m_que.empty();
    });
    CallerUPtr vcall = std::move(m_que.front());
    m_que.pop();
    return vcall;
}

void CallerQueue::releaseCaller(CallerUPtr vcall)
{
    std::unique_lock<std::mutex> lock(m_que_mtx);
    m_que.push(std::move(vcall));
    m_que_cond.notify_one();
}

/*
VCall
{
    "accounts": [
        {"sip_user": "", "sip_doamin": "", "sip_password": ""},
        {"sip_user": "", "sip_doamin": "", "sip_password": ""},
    ]
}
*/
// void CallerQueue::fetch()
// {
//     std::vector<AccountUPtr> vaccounts;
//     auto resp = voip::httpRequest(m_ioc, "localhost", "5000", "/accounts", voip::http::verb::get);

//     if (resp.isMember("accounts") && resp["accounts"].isArray()) {
//         for (const auto &item : resp["accounts"]) {
//             std::string sip_user = item["sip_user"].asString();
//             std::string sip_domain = item["sip_domain"].asString();
//             std::string sip_password = item["sip_password"].asString();
//             AccountUPtr acc = createAccount(sip_user, sip_domain, sip_password);
//             vaccounts.push_back(std::move(acc));
//         }
//     }
//     {
//         std::unique_lock<std::mutex> lock(m_que_mtx);
//         for (auto &vacc_uptr : vaccounts) {
//             auto vcall_uptr = createCall(std::move(vacc_uptr));
//             m_que.emplace(std::move(vcall_uptr));
//         }
//     }
// }

CallerQueue::AccountUPtr CallerQueue::createAccount(
    const std::string &sip_user,
    const std::string &sip_domain,
    const std::string &sip_password)
{
    pj::AuthCredInfo cred {"digest", "*", sip_user, 0, sip_password};
    pj::AccountConfig cfg;
    cfg.idUri = "sip:" + sip_user + "@" + sip_domain;
    cfg.regConfig.registrarUri = "sip:" + sip_domain;
    cfg.callConfig.timerMinSESec = 90;
    cfg.callConfig.timerSessExpiresSec = 1800;
    std::unique_ptr<voip::VAccount> account = std::make_unique<voip::VAccount>();
    account->create(cfg);
    return account;
}

CallerQueue::CallerUPtr CallerQueue::createCaller(AccountUPtr vaccount)
{
    CallerUPtr caller = std::make_unique<voip::Caller>(*vaccount);
    return caller;
}
