cmake_minimum_required(VERSION 3.10)

project(answer)

set(SRC 
    call.hpp
    account.hpp
    main.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE
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
    upnp 
    ixml 
    uuid 
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
    bcg729
)
