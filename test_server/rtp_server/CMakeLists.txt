cmake_minimum_required(VERSION 3.10)

project(rtp-server LANGUAGES CXX VERSION 1.0)

set(SRC
    main.cc
    rtp_server.cc
)

include_directories(/usr/local/include/jrtplib3)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    /usr/local/lib/libjrtp.so
)