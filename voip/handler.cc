#include "handler.h"
#include "http.h"
#include "core.h"

#include <boost/beast.hpp>
#include <json/json.h>
#include <iostream>

namespace beast = boost::beast;
namespace json = Json;

Handler::~Handler()
{
}

void Handler::registerGet(std::string path, callback cb)
{
    m_get[path] = cb;
}

bool Handler::handleGet(std::string path, std::shared_ptr<Http> conn)
{
    if (m_get.find(path) == m_post.end()) {
        return false;
    }
    m_get[path](conn);
    return true;
}

void Handler::registerPost(std::string path, callback cb)
{
    m_post[path] = cb;
}

bool Handler::handlePost(std::string path, std::shared_ptr<Http> conn)
{
    if (m_post.find(path) == m_post.end()) {
        return false;
    }
    m_post[path](conn);
    return true;
}

Handler::Handler()
{
    registerGet("/", [](std::shared_ptr<Http> conn) {
        beast::ostream(conn->m_response.body()) << "[GET] {/api} req";
        return;
    });
    registerPost("/call", [](std::shared_ptr<Http> conn) {
        auto body_str = beast::buffers_to_string(
            conn->m_request.body().data());
        std::cout << "" << body_str << std::endl;
        conn->m_response.set(
            http::field::content_type, "text/json");
        json::Value send_root;
        json::Value recv_root;
        json::Reader reader;
        if (!reader.parse(body_str, recv_root)) {
            std::cerr << "[Json Parse Error]" << std::endl;
            send_root["error"] = "Json Parse Error";
            beast::ostream(conn->m_response.body()) << send_root.toStyledString();
            return;
        }
        if (!recv_root.isMember("phone")) {
            std::cerr << "[Json Parse Error]" << std::endl;
            send_root["error"] = "Json Parse Error";
            beast::ostream(conn->m_response.body()) << send_root.toStyledString();
            return;
        }
        json::String phone = recv_root["phone"].asString();
        auto core = Core::getInstance();
        core->makeCall(phone);
        beast::ostream(conn->m_response.body()) << send_root.toStyledString();
        return;
    });
}
