cmake_minimum_required(VERSION 3.10)
project(SIPC_VoIP)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PJPROJECT_DIR /usr/local)

include_directories(${PROJECT_SOURCE_DIR})

link_directories(/usr/local/lib)

add_executable(voip 
    # voip.cpp 
    # vcall.cc
    # main.cc
    # test/offical.cc
    # test/main.cc
    # test/test_audiomediaport.cc
    vaudiomediaport.cc
    vaccount.cc
    vcall.cc
    voip.cc
)

target_link_libraries(voip
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

# g++ voip.cpp -L/usr/local/lib 
# -lpjsua2-x86_64-pc-linux-gnu 
# -lpjsua-x86_64-pc-linux-gnu 
# -lpjsip-ua-x86_64-pc-linux-gnu 
# -lpjsip-simple-x86_64-pc-linux-gnu 
# -lpjsip-x86_64-pc-linux-gnu 
# -lpjmedia-codec-x86_64-pc-linux-gnu 
# -lpjmedia-videodev-x86_64-pc-linux-gnu 
# -lpjmedia-audiodev-x86_64-pc-linux-gnu 
# -lpjmedia-x86_64-pc-linux-gnu 
# -lpjnath-x86_64-pc-linux-gnu 
# -lpjlib-util-x86_64-pc-linux-gnu 
# -lpj-x86_64-pc-linux-gnu 
# -lsrtp-x86_64-pc-linux-gnu 
# -lresample-x86_64-pc-linux-gnu 
# -lgsmcodec-x86_64-pc-linux-gnu 
# -lspeex-x86_64-pc-linux-gnu 
# -lilbccodec-x86_64-pc-linux-gnu 
# -lg7221codec-x86_64-pc-linux-gnu 
# -lyuv-x86_64-pc-linux-gnu 
# -lwebrtc-x86_64-pc-linux-gnu  
# -lopus 
# -lssl 
# -lcrypto 
# -lvpx 
# -lopenh264 
# -lstdc++ 
# -lupnp 
# -lixml 
# -luuid 
# -lm 
# -lrt 
# -lpthread  
# -lasound 
# -l/usr/lib 
# -lSDL2  
# -lavdevice 
# -lavformat 
# -lavcodec 
# -lswscale 
# -lavutil 
# -lv4l2 
# -lopencore-amrnb 
# -lopencore-amrwb
