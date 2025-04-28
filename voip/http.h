#ifndef _HTTP_H_
#define _HTTP_H_

#include <boost/beast.hpp>
#include <unordered_map>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = asio::ip::tcp;

class Http :
    public std::enable_shared_from_this<Http>
{
    friend class Handler;

public:
    Http(asio::io_context &ioc);

    tcp::socket &getSocket();
    void start();

private:
    void heartheat();
    void writeResponse();
    void handleRequest();

    unsigned char toHex(const unsigned char x) const;
    unsigned char fromHex(const unsigned char x) const;
    std::string encodeUrl(const std::string &url) const;
    std::string decodeUrl(const std::string &url) const;

    void parseParam();
    void parseForm();

private:
    tcp::socket m_socket;
    beast::flat_buffer m_buffer;
    http::request<http::dynamic_body> m_request;
    http::response<http::dynamic_body> m_response;
    asio::steady_timer m_deadline;
    std::string m_get_url;
    std::unordered_map<std::string, std::string> m_get_params;
};

#endif // _HTTP_H_