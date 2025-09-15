#ifndef _BRIDGE_H_
#define _BRIDGE_H_

#include "agent_ws_client.h"
#include "web_ws_client.h"
#include <memory>

class Bridge
{
public:
    Bridge(std::shared_ptr<AgentWsClient> agent_ws_client,
           std::shared_ptr<WebWsClient> web_ws_client) :
        m_agent_ws_client(std::move(agent_ws_client)),
        m_web_ws_client(std::move(web_ws_client))
    {
        if (m_agent_ws_client) {
            // m_agent_ws_client->set
        }
        if (m_web_ws_client) {
            // m_web_ws_client->set
        }
    }
    ~Bridge();

private:
    std::shared_ptr<AgentWsClient> m_agent_ws_client;
    std::shared_ptr<WebWsClient> m_web_ws_client;
};

#endif // _BRIDGE_H_