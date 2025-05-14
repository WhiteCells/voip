#ifndef _CORE_H_
#define _CORE_H_

#include "singleton.hpp"
#include "vaccount.h"

#include <pjsua2.hpp>
#include <memory>

class Core
{
public:
    Core();
    Core(const Core &) = delete;
    Core &operator=(const Core &) = delete;
    ~Core();

    void makeCall(const std::string &phone);

    void config(
        const std::string &sip_user = "1003",
        const std::string &sip_doamin = "192.168.10.51:5060",
        const std::string &sip_password = "1003",
        const unsigned int sip_port = 50601,
        const pjsip_transport_type_e ts_tp = PJSIP_TRANSPORT_UDP);

private:
    // std::shared_ptr<voip::VAccount> createAccount()

private:
    std::string m_sip_user;
    std::string m_sip_domain;
    std::string m_sip_password;
    unsigned int m_sip_port;
    pjsip_transport_type_e m_ts_tp;

    pj::Endpoint m_endpoint;
    pj::EpConfig m_endpoint_cfg;
    pj::TransportConfig m_transport_cfg;
    std::shared_ptr<voip::VAccount> m_account;
    pj::AccountConfig m_account_cfg;
    pj::AuthCredInfo m_auth_cred_info;
    voip::Caller *m_vcall;
};

#endif // _CORE_H_