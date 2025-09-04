cmake_minimum_required(VERSION 3.10)

project(client LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(3rd/spdlog)

set(SRC
    src/main.cc
    src/rtp_session_impl.h
    src/rtp_session_impl.cc
    src/rtp_session_mgr.h
    src/rtp_session_mgr.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_include_directories(${PROJECT_NAME} PRIVATE
    3rd/spdlog/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog_header_only
)