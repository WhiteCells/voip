#include "http.h"
#include "handler.h"

#include <iostream>

Http::Http(asio::io_context &ioc) :
    m_socket(ioc),
    m_buffer {8192},
    m_deadline({m_socket.get_executor(),
                std::chrono::seconds(60)})
{
}

tcp::socket &Http::getSocket()
{
    return m_socket;
}

void Http::start()
{
    auto self = shared_from_this();
    http::async_read(
        m_socket,
        m_buffer,
        m_request,
        [self](beast::error_code ec, std::size_t transfer_byte) {
            boost::ignore_unused(transfer_byte);
            try {
                if (ec) {
                    std::cerr << "[Err]: " << ec.what() << std::endl;
                    return;
                }
                self->handleRequest();
                self->heartheat();
            }
            catch (const std::exception &e) {
                std::cerr << "[Exception]: " << e.what() << std::endl;
            }
        });
}

void Http::heartheat()
{
    auto self = shared_from_this();
    m_deadline.async_wait(
        [self](beast::error_code ec) {
            if (!ec) {
                ec = self->m_socket.close(ec);
            }
        });
}

void Http::writeResponse()
{
    auto self = shared_from_this();
    m_response.content_length(m_response.body().size());
    http::async_write(
        m_socket,
        m_response,
        [self](beast::error_code ec, std::size_t transfer_byte) {
            boost::ignore_unused(transfer_byte);
            ec = self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
            self->m_deadline.cancel();
        });
}

void Http::handleRequest()
{
    m_response.version(m_request.version());
    m_response.keep_alive(false);

    std::cout << "[Request Path]: " << m_request.target() << std::endl;

    switch (m_request.method()) {
        case http::verb::get: {
            std::cout << "[Route Get]" << std::endl;
            bool ok = Handler::getInstance()->handleGet(
                m_request.target(), shared_from_this());
            if (!ok) {
                m_response.result(http::status::not_found);
                m_response.set(http::field::content_type, "text/json");
                beast::ostream(m_response.body()) << "url nof found\r\n";
                writeResponse();
                return;
            }
            m_response.result(http::status::ok);
            m_response.set(http::field::server, "serve");
            writeResponse();
            break;
        }

        case http::verb::post: {
            std::cout << "[Route Post]" << std::endl;
            bool ok = Handler::getInstance()->handlePost(
                m_request.target(), shared_from_this());
            if (!ok) {
                m_response.result(http::status::not_found);
                m_response.set(http::field::content_type, "text/json");
                beast::ostream(m_response.body()) << "url nof found\r\n";
                writeResponse();
                return;
            }
            m_response.result(http::status::ok);
            m_response.set(http::field::server, "serve");
            writeResponse();
            break;
        }

        case http::verb::delete_: {
            std::cout << "[Route Delete]" << std::endl;
            bool ok = Handler::getInstance()->handleDelete(
                m_request.target(), shared_from_this());
            if (!ok) {
                m_response.result(http::status::not_found);
                m_response.set(http::field::content_type, "text/json");
                beast::ostream(m_response.body()) << "url nof found\r\n";
                writeResponse();
                return;
            }
            m_response.result(http::status::ok);
            m_response.set(http::field::server, "serve");
            writeResponse();
            break;
        }

        case http::verb::put: {
            std::cout << "[Route Put]" << std::endl;
            bool ok = Handler::getInstance()->handlePut(
                m_request.target(), shared_from_this());
            if (!ok) {
                m_response.result(http::status::not_found);
                m_response.set(http::field::content_type, "text/json");
                beast::ostream(m_response.body()) << "url nof found\r\n";
                writeResponse();
                return;
            }
            m_response.result(http::status::ok);
            m_response.set(http::field::server, "serve");
            writeResponse();
            break;
        }

        default:
            break;
    }
}

unsigned char Http::toHex(const unsigned char x) const
{
    return x > 9 ? x + 55 : x + 48;
}

unsigned char Http::fromHex(const unsigned char x) const
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') {
        y = x - 'A' + 10;
    }
    else if (x >= 'a' && x <= 'z') {
        y = x - 'a' + 10;
    }
    else if (x >= '0' && x <= '9') {
        y = x - '0';
    }
    else {
        assert(0);
    }
    return y;
}

std::string Http::encodeUrl(const std::string &url) const
{
    std::string res {};
    for (std::size_t i = 0; i < url.size(); ++i) {
        if (std::isalnum(url[i]) ||
            url[i] == '-' ||
            url[i] == '_' ||
            url[i] == '.' ||
            url[i] == '~') {
            res += url[i];
        }
        else if (url[i] == ' ') {
            res += '+';
        }
        else {
            res += '%';
            res += toHex(url[i] >> 4);
            res += toHex(url[i] & 0x0F);
        }
    }
    return res;
}

std::string Http::decodeUrl(const std::string &url) const
{
    std::string res {};
    for (std::size_t i = 0; i < url.size(); ++i) {
        if (url[i] == '+') {
            res += ' ';
        }
        else if (url[i] == '%') {
            unsigned char high = fromHex(url[++i]);
            unsigned char low = fromHex(url[++i]);
            res += high * 16 + low;
        }
        else {
            res += url[i];
        }
    }
    return res;
}

void Http::parseParam()
{
}

void Http::parseForm()
{
}
