set(SRC
    # audio_dev.cc
    # audio_queue.cc
    # rtp_server.cc
    main.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE portaudio 
    /usr/local/lib/libjrtp.so
    opus
)