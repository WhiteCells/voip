#define BOOST_TEST_MODULE REQUEST_TEST

#include "../request.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <thread>

namespace asio = boost::asio;

BOOST_AUTO_TEST_CASE(request_heartbeat)
{
    // auto res = voip::httpRequest("localhost", "5000", "/heartbeat/123", voip::http::verb::post);

    asio::io_context ioc;
    voip::heartbeatHttpPoll(ioc, "123");

    std::thread io_thread {[&]() {
        ioc.run();
    }};

    std::this_thread::sleep_for(std::chrono::seconds {10});
    io_thread.join();
    ioc.stop();

    // BOOST_CHECK(res);
}