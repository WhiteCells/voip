#ifndef _WEB_AGENT_BRIDGE_H_
#define _WEB_AGENT_BRIDGE_H_

#include "agent_ws_client.h"
#include "web_ws_client.h"
#include <memory>

class WebAgentBridge
{
public:
    WebAgentBridge(std::shared_ptr<AgentWsClient> agent_ws_client,
                   std::shared_ptr<WebWsClient> web_ws_client)
        : m_agent_ws_client(std::move(agent_ws_client))
        , m_web_ws_client(std::move(web_ws_client))
    {
        if (m_agent_ws_client) {
            m_agent_ws_client->set_web_ws_sender([this](const std::string &msg) {
                m_web_ws_client->send(msg);
            });
        }
        if (m_web_ws_client) {
            m_web_ws_client->set_agent_ws_sender([this](const std::string &msg) {
                m_agent_ws_client->send(msg);
            });
        }
    }
    ~WebAgentBridge() {}

private:
    std::shared_ptr<AgentWsClient> m_agent_ws_client;
    std::shared_ptr<WebWsClient> m_web_ws_client;
};

#endif // _WEB_AGENT_BRIDGE_H_
