cmake_minimum_required(VERSION 3.10)

project(asr-ws-client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC
    src/main.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} 
    jsoncpp
    portaudio
    ssl
    crypto
)