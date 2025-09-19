#define BOOST_TEST_MODULE ASYNC_TIMER_TEST

#include "../voip/async_timer.h"

#include <boost/test/unit_test.hpp>
#include <boost/asio.hpp>
#include <thread>

BOOST_AUTO_TEST_CASE(timer_start)
{
    boost::asio::io_context ioc;
    AsyncTimer timer(ioc, std::chrono::seconds {1});
    bool flag = false;
    timer.start(std::bind(
        [&flag](int _) {
            flag = true;
        },
        1));
    std::thread io_thread {[&ioc]() {
        ioc.run();
    }};
    std::this_thread::sleep_for(std::chrono::seconds {2});
    timer.stop();
    io_thread.join();
    ioc.stop();

    BOOST_CHECK(flag);
}
