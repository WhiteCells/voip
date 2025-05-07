cmake_minimum_required(VERSION 3.10)
project(voip)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PJPROJECT_DIR /usr/local)

include_directories(${PROJECT_SOURCE_DIR})

link_directories(/usr/local/lib)

add_subdirectory(third_party/jsoncpp)

add_executable(${PROJECT_NAME} 
    # test/test_audiomedia.cc
    # voip.cpp 
    # vcall.cc
    # main.cc
    # test/offical.cc
    # test/main.cc
    # test/test_audiomediaport.cc

    request.hpp
    vaccount.cc
    vaudiomediaport.cc
    vcall.cc
    
    core.cc
    handler.cc
    http.cc
    io_context_pool.cc
    main.cc
    server.cc
    singleton.hpp

    ini.cc
    # voip.cc
)

target_link_libraries(voip
    jsoncpp_lib
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
