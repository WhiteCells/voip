#include "voipcore.h"

#include <QMessageBox>

VoipCore::VoipCore() :
    m_endpoint(std::make_shared<pj::Endpoint>()),
    m_account(std::make_shared<VAccount>())
{
    startEndpointLib();
    createAccount();
}

VoipCore::~VoipCore()
{
    if (m_endpoint) {
        m_endpoint->libDestroy();
    }
}

void VoipCore::slot_call_out(const QString phone_num)
{
    m_call = std::make_shared<VCall>(*m_account);
    pj::CallOpParam prm(true);
    QString dst_url = "sip:" + phone_num + "@192.168.10.51:5060";
    m_call->makeCall(dst_url.toStdString(), prm);
}

void VoipCore::startEndpointLib()
{
    m_endpoint->libCreate();
    pj::EpConfig ep_cfg;
    m_endpoint->libInit(ep_cfg);
    pj::TransportConfig ts_cfg;
    ts_cfg.port = 5060;
    m_endpoint->transportCreate(PJSIP_TRANSPORT_UDP, ts_cfg);
    m_endpoint->libStart();
}

void VoipCore::createAccount()
{
    pj::AccountConfig acc_cfg;
    acc_cfg.idUri = "sip:"
                    "1003"
                    "@"
                    "192.168.10.51:5060";
    acc_cfg.regConfig.registrarUri = "sip:"
                                     "192.168.10.51:5060";
    pj::AuthCredInfo cred("digest", "*", "1003", 0, "1003");
    acc_cfg.sipConfig.authCreds.push_back(cred);
    m_account->create(acc_cfg);
}
