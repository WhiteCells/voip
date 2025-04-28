#include "handler.h"

#include <boost/beast.hpp>

namespace beast = boost::beast;

Handler::~Handler()
{
}

void Handler::registerGet(std::string path, callback cb)
{
    m_get[path] = cb;
}

bool Handler::handleGet(std::string path, std::shared_ptr<Http> conn)
{
    if (!m_get.contains(path)) {
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
    if (!m_post.contains(path)) {
        return false;
    }
    m_post[path](conn);
    return true;
}

Handler::Handler()
{
    registerGet("/api", [](std::shared_ptr<Http>) {
        beast::ostream(conn->)
    });
}
