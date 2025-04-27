#ifndef _VOIPCORE_H_
#define _VOIPCORE_H_

#include "vaccount.h"
#include "vcall.h"

#include <pjsua2.hpp>
#include <QObject>
#include <memory>

using namespace voip;

class VoipCore : public QObject
{
    Q_OBJECT
public:
    VoipCore();
    ~VoipCore();

public slots:
    void slot_call_out(const QString phone_num);

private:
    void startEndpointLib();
    void createAccount();

private:
    std::shared_ptr<pj::Endpoint> m_endpoint;
    std::shared_ptr<VAccount> m_account;
    std::shared_ptr<VCall> m_call;
};

#endif // _VOIPCORE_H_