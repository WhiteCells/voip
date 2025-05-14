#include "core.h"
#include "caller.h"

#include <iostream>

Core::Core()
{
    config();
}

Core::~Core()
{
    m_endpoint.libDestroy();
    if (m_vcall) {
        delete m_vcall;
        m_vcall = nullptr;
    }
}

void Core::makeCall(const std::string &phone)
{
    if (!m_vcall) {
        m_vcall = new voip::Caller(*m_account);
    }
    std::string dst_uri = "sip:" + phone + "@" + m_sip_domain;
    pj::CallOpParam prm(true);
    m_vcall->makeCall(dst_uri, prm);
    if (!m_account) {
        m_account->cur_call = m_vcall;
    }
}

void Core::config(
    const std::string &sip_user,
    const std::string &sip_domain,
    const std::string &sip_password,
    const unsigned int sip_port,
    const pjsip_transport_type_e ts_tp)
{
    m_sip_user = sip_user;
    m_sip_domain = sip_domain;
    m_sip_password = sip_password;
    m_sip_port = sip_port;
    m_ts_tp = ts_tp;

    m_endpoint.libCreate();
    m_endpoint.libInit(m_endpoint_cfg);
    m_transport_cfg.port = sip_port;
    m_endpoint.transportCreate(ts_tp, m_transport_cfg);
    m_endpoint.libStart();

    m_auth_cred_info = pj::AuthCredInfo("digest",
                                        "*",
                                        sip_user,
                                        0,
                                        sip_password);
    m_account_cfg.idUri = "sip:" + sip_user + "@" + sip_domain;
    m_account_cfg.regConfig.registrarUri = "sip:" + sip_domain;
    m_account_cfg.sipConfig.authCreds.push_back(m_auth_cred_info);
    m_account_cfg.callConfig.timerMinSESec = 90;
    m_account_cfg.callConfig.timerSessExpiresSec = 1800;
    std::cout << "SIP Config: idUri = " << m_account_cfg.idUri
              << ", registrarUri = " << m_account_cfg.regConfig.registrarUri
              << std::endl;

    m_account = std::make_shared<voip::VAccount>();
    m_account->create(m_account_cfg);
}


