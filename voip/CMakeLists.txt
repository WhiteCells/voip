set(SRC 
    agent_audiomediaport.cc
    global.cc
    async_timer.cc
    dialplan_queue.cc
    caller_queue.cc
    caller_vec.cc
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
    logger.cc
    main.cc
)

include_directories(
    ${Boost_INCLUDE_DIRS}
    /usr/include/jsoncpp
    /usr/local/include/jrtplib3
)
# include_directories(/usr/include/jsoncpp)
# include_directories(/usr/local/include/jrtplib3)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    /usr/local/lib/libjrtp.so
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    spdlog::spdlog_header_only
    jsoncpp
    ${Boost_LIBRARIES}
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
)
