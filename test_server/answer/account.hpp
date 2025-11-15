#pragma once

#include "call.hpp"
#include <pjsua2.hpp>
#include <iostream>
#include <pjsua2/call.hpp>

using namespace pj;

class MyAccount : public Account
{
public:
    virtual void onIncomingCall(OnIncomingCallParam &iprm) override
    {
        std::cout << "Incoming call detected!" << std::endl;

        m_cur_call = new MyCall(*this, iprm.callId);

        CallOpParam prm;
        prm.statusCode = PJSIP_SC_OK; // 200
        // call->answer(prm);
        // std::cout << "Call answered automatically." << std::endl;
        std::cout << "Call ring" << std::endl;
        // to answer
        answerCurCall();
    }

    void answerCurCall()
    {
        if (m_cur_call) {
            CallOpParam prm;
            prm.statusCode = PJSIP_SC_OK;
            m_cur_call->answer(prm);
            std::cout << "answer" << std::endl;
        }
    }

private:
    Call *m_cur_call;
};