#ifndef _LOCALHOST_H_
#define _LOCALHOST_H_

#include <boost/asio.hpp>
#include <string>
#include <iostream>

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

inline std::string localhostV4Str()
{
    asio::io_context ioc;
    tcp::resolver resovler(ioc);
    auto endpoints = resovler.resolve(asio::ip::host_name(), "");
    for (const auto &endpoint : endpoints) {
        asio::ip::address addr = endpoint.endpoint().address();
        if (addr.is_v4()) {
            std::cout << addr.to_string() << std::endl;
            // return addr.to_string();
        }
    }
    return "";
}

#endif // _LOCALHOST_H_