#define BOOST_TEST_MODULE THREAD_POOL_TEST

#include "../voip/thread_pool.h"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_thread_pool)
{
    auto pool = ThreadPool {};
    for (int i = 0; i < 10; ++i) {
        pool.addTask(
            [=](int i) {
                std::cout << ">>>" << i << std::endl;
            },
            i);
    }
}