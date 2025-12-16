#pragma once

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/beast.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/ssl.hpp>

// namespace beast = boost::beast;
// namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

// 异步 HTTP 客户端
// 推送分机号注册状态
// 推送呼叫状态
// 推送群呼状态结束
class WebHttpClient
{
public:
    // WebHttpClient(net::io_context &ioc,
    //               const std::string &host,
    //               const std::string &port,
    //               const std::string &paht,
    //               const std::string &ssl_cert_file)
    // {
    // }

    ~WebHttpClient()
    {
    }

private:
};

// static inline void pushAccountRegState(net::io_context &ioc,
//                                        const std::string &host,
//                                        const std::string &port,
//                                        const std::string &path,
//                                        const std::string &ssl_cert_file,
//                                        const std::string &account_id,
//                                        bool is_registered)
// {
// }

// static inline void pushCallState(net::io_context &ioc,
//                                  const std::string &host,
//                                  const std::string &port,
//                                  const std::string &path,
//                                  const std::string &ssl_cert_file,
//                                  const std::string &call_id,
//                                  const std::string &state)
// {
// }

// static inline void pushGroupCallFinished(net::io_context &ioc,
//                                          const std::string &host,
//                                          const std::string &port,
//                                          const std::string &path,
//                                          const std::string &ssl_cert_file,
//                                          const std::string &call_id,
//                                          const std::string &state)
// {
// }
