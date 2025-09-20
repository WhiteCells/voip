set(SRC
    main.cc
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    /usr/local/lib/libjrtp.so
    portaudio 
    opus
)