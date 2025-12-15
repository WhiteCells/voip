cmake_minimum_required(VERSION 3.10)

project(asr_ws_client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC
    src/logger.cc
    src/main.cc
)

add_subdirectory(../../3rd/spdlog spdlog)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} spdlog portaudio jsoncpp ssl crypto)
