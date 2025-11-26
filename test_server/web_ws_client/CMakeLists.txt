cmake_minimum_required(VERSION 3.10)

project(web_ws_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC
    src/web_ws_client.cc
    src/logger.cc
    src/io_context_pool.cc
    src/main.cc
)

find_package(spdlog REQUIRED)
# add_subdirectory(../../third_party/spdlog)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog_header_only
    jsoncpp
    ssl
    crypto 
)
