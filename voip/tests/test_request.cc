#define BOOST_TEST_MODULE REQUEST_TEST

#include "../request.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <thread>

namespace asio = boost::asio;

BOOST_AUTO_TEST_CASE(request_heartbeat)
{
    asio::io_context ioc;
    voip::heartbeatRequest(ioc, "123");

    std::thread io_thread {[&]() {
        ioc.run();
    }};

    std::this_thread::sleep_for(std::chrono::seconds {10});
    io_thread.join();
    ioc.stop();
}