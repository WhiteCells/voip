cmake_minimum_required(VERSION 3.10)
project(voip)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party/jsoncpp)
# add_subdirectory(tests)

set(SRC 
    global.cc
    async_timer.cc
    dialplan_queue.cc
    caller_queue.cc
    caller.cc
    vaccount.cc
    client.cc
    request.hpp
    io_context_pool.cc
    server.cc
    ini.cc
    http.cc
    handler.cc
    singleton.hpp
    thread_pool.cc
    vaudiomediaport.cc
    main.cc
)

include_directories(${PROJECT_SOURCE_DIR})

# add_library(${PROJECT_NAME}_lib STATIC
#     ${SRC}
# )

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE
    jsoncpp
    pjsua2-x86_64-pc-linux-gnu 
    pjsua-x86_64-pc-linux-gnu 
    pjsip-ua-x86_64-pc-linux-gnu 
    pjsip-simple-x86_64-pc-linux-gnu 
    pjsip-x86_64-pc-linux-gnu 
    pjmedia-codec-x86_64-pc-linux-gnu 
    pjmedia-videodev-x86_64-pc-linux-gnu 
    pjmedia-audiodev-x86_64-pc-linux-gnu 
    pjmedia-x86_64-pc-linux-gnu 
    pjnath-x86_64-pc-linux-gnu 
    pjlib-util-x86_64-pc-linux-gnu 
    pj-x86_64-pc-linux-gnu 
    srtp-x86_64-pc-linux-gnu 
    resample-x86_64-pc-linux-gnu 
    gsmcodec-x86_64-pc-linux-gnu 
    speex-x86_64-pc-linux-gnu 
    ilbccodec-x86_64-pc-linux-gnu 
    g7221codec-x86_64-pc-linux-gnu 
    yuv-x86_64-pc-linux-gnu 
    webrtc-x86_64-pc-linux-gnu  
    opus 
    ssl 
    crypto 
    vpx 
    openh264 
    stdc++ 
    upnp 
    ixml 
    uuid 
    m 
    rt 
    pthread  
    asound 
    avdevice 
    avformat 
    avcodec 
    swscale 
    avutil 
    v4l2 
    opencore-amrnb 
    opencore-amrwb
)