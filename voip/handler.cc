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

void Handler::registerDelete(std::string path, callback cb)
{
    m_delete[path] = cb;
}

bool Handler::handleDelete(std::string path, std::shared_ptr<Http> conn)
{
    if (m_delete.find(path) == m_delete.end()) {
        return false;
    }
    m_delete[path](conn);
    return true;
}

void Handler::registerPut(std::string path, callback cb)
{
    m_put[path] = cb;
}

bool Handler::handlePut(std::string path, std::shared_ptr<Http> conn)
{
    if (m_put.find(path) == m_put.end()) {
        return false;
    }
    m_put[path](conn);
    return true;
}

Handler::Handler()
{
    registerGet("/", [](std::shared_ptr<Http> conn) {
        beast::ostream(conn->m_response.body()) << "[GET] {/api} req";
    });
    registerGet("/status", [](std::shared_ptr<Http> conn) {
        conn->m_response.set(http::field::content_type, "text/json");
        json::Value send_root;
        send_root["status"] = 0;
        beast::ostream(conn->m_response.body()) << send_root.toStyledString();
    });
    registerPost("/call", [](std::shared_ptr<Http> conn) {
        auto core = Core::getInstance();
        pj::Endpoint::instance().libRegisterThread("beast_http");
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
            std::cerr << "[Json Field Error]" << std::endl;
            send_root["error"] = "Json Field Error";
            beast::ostream(conn->m_response.body()) << send_root.toStyledString();
            return;
        }
        json::String phone = recv_root["phone"].asString();
        send_root["status"] = 1;
        beast::ostream(conn->m_response.body()) << send_root.toStyledString();
        core->makeCall(phone);
    });
}
