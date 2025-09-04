cmake_minimum_required(VERSION 3.10)

project(server LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(3rd/spdlog)

set(SRC
    src/main.cc
    src/ws_server.h
    src/ws_server.cc
    src/ws_session.h
    src/ws_session.cc
    src/singleton.hpp
    src/io_context_pool.h
    src/io_context_pool.cc
    src/ws_session_mgr.h
    src/ws_session_mgr.cc
    src/logger.h
    src/logger.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE)

target_link_directories(${PROJECT_NAME} PRIVATE 
    3rd/spdlog/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog
)