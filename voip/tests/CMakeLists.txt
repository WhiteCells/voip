find_package(Boost REQUIRED COMPONENTS unit_test_framework)

set(TEST_SOURCES 
    test_async_timer.cc
    test_core.cc
    test_request.cc
    test_thread_pool.cc
)

add_library(src_lib STATIC 
    ../async_timer.cc 
    ../core.cc
    ../request.hpp
    ../thread_pool.h
    ../thread_pool.cc
)

foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})
    target_link_libraries(${TEST_NAME}
        jsoncpp_lib
        src_lib
        Boost::unit_test_framework
    )
endforeach()
