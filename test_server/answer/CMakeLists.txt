#cmake_minimum_required(VERSION 3.10)
#
#project(answer)
#
#set(SRC
#    call.hpp
#    account.hpp
#    main.cc
#)
#
#add_executable(${PROJECT_NAME} ${SRC})
#
#target_link_libraries(${PROJECT_NAME} PRIVATE
#    pjsua2-x86_64-pc-linux-gnu
#    pjsua-x86_64-pc-linux-gnu
#    pjsip-ua-x86_64-pc-linux-gnu
#    pjsip-simple-x86_64-pc-linux-gnu
#    pjsip-x86_64-pc-linux-gnu
#    pjmedia-codec-x86_64-pc-linux-gnu
#    pjmedia-videodev-x86_64-pc-linux-gnu
#    pjmedia-audiodev-x86_64-pc-linux-gnu
#    pjmedia-x86_64-pc-linux-gnu
#    pjnath-x86_64-pc-linux-gnu
#    pjlib-util-x86_64-pc-linux-gnu
#    pj-x86_64-pc-linux-gnu
#    srtp-x86_64-pc-linux-gnu
#    resample-x86_64-pc-linux-gnu
#    gsmcodec-x86_64-pc-linux-gnu
#    speex-x86_64-pc-linux-gnu
#    ilbccodec-x86_64-pc-linux-gnu
#    g7221codec-x86_64-pc-linux-gnu
#    yuv-x86_64-pc-linux-gnu
#    webrtc-x86_64-pc-linux-gnu
#    opus
#    ssl
#    crypto
#    vpx
#    upnp
#    ixml
#    uuid
#    rt
#    pthread
#    asound
#    avdevice
#    avformat
#    avcodec
#    swscale
#    avutil
#    v4l2
#    opencore-amrnb
#    opencore-amrwb
#    bcg729
#)


cmake_minimum_required(VERSION 3.10)

project(voip LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#configure_file(${CMAKE_SOURCE_DIR}/.env ${CMAKE_BINARY_DIR}/voip/Debug/.env COPYONLY)
#configure_file(${CMAKE_SOURCE_DIR}/crt/agent-session-server.crt ${CMAKE_BINARY_DIR}/voip/Debug/agent-session-server.crt COPYONLY)
#configure_file(${CMAKE_SOURCE_DIR}/crt/asr-server.crt ${CMAKE_BINARY_DIR}/voip/Debug/asr-server.crt COPYONLY)
#configure_file(${CMAKE_SOURCE_DIR}/crt/backend-server.crt ${CMAKE_BINARY_DIR}/voip/Debug/backend-server.crt COPYONLY)

option(VOIP_SSL "Enable SSL")
option(FEATURE "Feature to build: REMINDER or ROBOT" "REMINDER")

set(FEATURE "REMINDER")
set(VOIP_SSL ON)
#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/voip)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/voip)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/voip)

if(WIN32)
    if (MSVC)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        add_compile_options(/bigobj)
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif()

    if(POLICY CMP0167)
        cmake_policy(SET CMP0167 NEW)
    endif()

    # 根据配置类型设置路径
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BOOST_ROOT           "D:/third_party_libs/boost")
        set(BOOST_LIBRARY_DIR    "${BOOST_ROOT}/stage/lib")

        set(SPDLOG_INCLUDE_DIR   "D:/third_party_libs/spdlog/include")
        set(SPDLOG_LIBRARY_DIR   "D:/third_party_libs/spdlog/build2/Debug")

        set(JSONCPP_INCLUDE_DIR  "D:/third_party_libs/jsoncpp/include")
        set(JSONCPP_LIBRARY_DIR  "D:/third_party_libs/jsoncpp/build2/lib/Debug")

        set(JRTPLIB_INCLUDE_DIR  "D:/third_party_libs/JRTPLIB/src")
        set(JRTPLIB_LIBRARY_DIR  "D:/third_party_libs/JRTPLIB/build2/src/Debug")

        set(OPUSLIB_INCLUDE_DIR  "D:/third_party_libs/opus-1.5.2/include")
        set(OPUSLIB_LIBRARY_DIR  "D:/third_party_libs/opus-1.5.2/build_debug/Debug")

        set(PJPROJECT_DIR        "D:/third_party_libs/pjproject")
        set(PJSIP_INCLUDE_DIRS
                "${PJPROJECT_DIR}/pjsip/include"
                "${PJPROJECT_DIR}/pjlib/include"
                "${PJPROJECT_DIR}/pjlib-util/include"
                "${PJPROJECT_DIR}/pjmedia/include"
                "${PJPROJECT_DIR}/pjnath/include"
        )

        set(PJPROJECT_PJSIP_LIBRARY_DIR      "${PJPROJECT_DIR}/pjsip/lib")
        set(PJPROJECT_PJLIB_LIBRARY_DIR      "${PJPROJECT_DIR}/pjlib/lib")
        set(PJPROJECT_PJLIB_UTIL_LIBRARY_DIR "${PJPROJECT_DIR}/pjlib-util/lib")
        set(PJPROJECT_PJMEDIA_LIBRARY_DIR    "${PJPROJECT_DIR}/pjmedia/lib")
        set(PJPROJECT_PJNATH_LIBRARY_DIR     "${PJPROJECT_DIR}/pjnath/lib")
        set(PJPROJECT_THIRD_PARTY_LIB_DIR    "${PJPROJECT_DIR}/third_party/lib")
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        set(BOOST_ROOT           "D:/third_party_libs/boost")
        set(BOOST_LIBRARY_DIR    "${BOOST_ROOT}/stage/lib")

        set(SPDLOG_INCLUDE_DIR   "D:/third_party_libs/spdlog/include")
        set(SPDLOG_LIBRARY_DIR   "D:/third_party_libs/spdlog/build/Release")

        set(JSONCPP_INCLUDE_DIR  "D:/third_party_libs/jsoncpp/include")
        set(JSONCPP_LIBRARY_DIR  "D:/third_party_libs/jsoncpp/build/lib/Release")

        set(JRTPLIB_INCLUDE_DIR  "D:/third_party_libs/JRTPLIB/src")
        set(JRTPLIB_LIBRARY_DIR  "D:/third_party_libs/JRTPLIB/build/src/Release")

        set(OPUSLIB_INCLUDE_DIR  "D:/third_party_libs/opus-1.5.2/include")
        set(OPUSLIB_LIBRARY_DIR  "D:/third_party_libs/opus-1.5.2/build_release/Release")

        set(PJPROJECT_DIR        "D:/third_party_libs/pjproject")
        set(PJSIP_INCLUDE_DIRS
                "${PJPROJECT_DIR}/pjsip/include"
                "${PJPROJECT_DIR}/pjlib/include"
                "${PJPROJECT_DIR}/pjlib-util/include"
                "${PJPROJECT_DIR}/pjmedia/include"
                "${PJPROJECT_DIR}/pjnath/include"
        )

        set(PJPROJECT_PJSIP_LIBRARY_DIR      "${PJPROJECT_DIR}/pjsip/lib")
        set(PJPROJECT_PJLIB_LIBRARY_DIR      "${PJPROJECT_DIR}/pjlib/lib")
        set(PJPROJECT_PJLIB_UTIL_LIBRARY_DIR "${PJPROJECT_DIR}/pjlib-util/lib")
        set(PJPROJECT_PJMEDIA_LIBRARY_DIR    "${PJPROJECT_DIR}/pjmedia/lib")
        set(PJPROJECT_PJNATH_LIBRARY_DIR     "${PJPROJECT_DIR}/pjnath/lib")
        set(PJPROJECT_THIRD_PARTY_LIB_DIR    "${PJPROJECT_DIR}/third_party/lib")
    else()
        message(FATAL_ERROR "Unknown build type: " ${CMAKE_BUILD_TYPE})
    endif()

    message(STATUS "Build configuration: " ${CMAKE_BUILD_TYPE})

    set(SRC
            call.hpp
            account.hpp
            main.cc
    )

    find_package(Boost REQUIRED COMPONENTS system filesystem)
    add_executable(${PROJECT_NAME} ${SRC})

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${Boost_INCLUDE_DIRS}
            ${SPDLOG_INCLUDE_DIR}
            ${JSONCPP_INCLUDE_DIR}
            ${PJSIP_INCLUDE_DIRS}
            ${JRTPLIB_INCLUDE_DIR}
            D:/third_party_libs/JRTPLIB/build/src
            D:/third_party_libs/JThread/src
            ${OPUSLIB_INCLUDE_DIR}
    )

    target_link_directories(${PROJECT_NAME} PRIVATE
            ${BOOST_LIBRARY_DIR}
            ${SPDLOG_LIBRARY_DIR}
            ${JSONCPP_LIBRARY_DIR}
            ${PJPROJECT_PJSIP_LIBRARY_DIR}
            ${PJPROJECT_PJLIB_LIBRARY_DIR}
            ${PJPROJECT_PJLIB_UTIL_LIBRARY_DIR}
            ${PJPROJECT_PJMEDIA_LIBRARY_DIR}
            ${PJPROJECT_PJNATH_LIBRARY_DIR}
            ${PJPROJECT_THIRD_PARTY_LIB_DIR}
            ${JRTPLIB_LIBRARY_DIR}
            ${OPUSLIB_LIBRARY_DIR}
    )

    # 根据配置类型设置不同的库文件
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_link_libraries(${PROJECT_NAME} PRIVATE
                # Boost
                ${Boost_LIBRARIES}

                # JsonCpp
                D:/third_party_libs/jsoncpp/build2/lib/Debug/jsoncpp_static.lib

                # PJSIP - Debug版本
                pjsip-core-x86_64-x64-vc14-Debug.lib
                pjsip-simple-x86_64-x64-vc14-Debug.lib
                pjsip-ua-x86_64-x64-vc14-Debug.lib
                pjsua-lib-x86_64-x64-vc14-Debug.lib
                pjsua2-lib-x86_64-x64-vc14-Debug.lib

                # PJLIB - Debug版本
                pjlib-x86_64-x64-vc14-Debug.lib
                pjlib-util-x86_64-x64-vc14-Debug.lib

                # PJMEDIA - Debug版本
                pjmedia-audiodev-x86_64-x64-vc14-Debug.lib
                pjmedia-codec-x86_64-x64-vc14-Debug.lib
                pjmedia-videodev-x86_64-x64-vc14-Debug.lib
                pjmedia-x86_64-x64-vc14-Debug.lib

                # PJNATH - Debug版本
                pjnath-x86_64-x64-vc14-Debug.lib

                # Third party - Debug版本
                libbaseclasses-x86_64-x64-vc14-Debug.lib
                libg7221codec-x86_64-x64-vc14-Debug.lib
                libgsmcodec-x86_64-x64-vc14-Debug.lib
                libilbccodec-x86_64-x64-vc14-Debug.lib
                libmilenage-x86_64-x64-vc14-Debug.lib
                libresample-x86_64-x64-vc14-Debug.lib
                libspeex-x86_64-x64-vc14-Debug.lib
                libsrtp-x86_64-x64-vc14-Debug.lib
                libwebrtc-x86_64-x64-vc14-Debug.lib
                libyuv-x86_64-x64-vc14-Debug.lib

                # JRTPLIB - Debug版本
                jrtplib_d.lib

                D:/third_party_libs/JThread/build2/src/Debug/jthread_d.lib

                # System
                ws2_32.lib

                # openssl - Debug版本
                D:/third_party_libs/OpenSSL-Win64_/lib/VC/x64/MTd/libcrypto.lib
                D:/third_party_libs/OpenSSL-Win64_/lib/VC/x64/MTd/libssl.lib

                opus.lib
        )
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE
                # Boost
                ${Boost_LIBRARIES}

                # JsonCpp - Release版本
                D:/third_party_libs/jsoncpp/build/lib/Release/jsoncpp_static.lib

                # PJSIP - Release版本
                pjsip-core-x86_64-x64-vc14-Release.lib
                pjsip-simple-x86_64-x64-vc14-Release.lib
                pjsip-ua-x86_64-x64-vc14-Release.lib
                pjsua-lib-x86_64-x64-vc14-Release.lib
                pjsua2-lib-x86_64-x64-vc14-Release.lib

                # PJLIB - Release版本
                pjlib-x86_64-x64-vc14-Release.lib
                pjlib-util-x86_64-x64-vc14-Release.lib

                # PJMEDIA - Release版本
                pjmedia-audiodev-x86_64-x64-vc14-Release.lib
                pjmedia-codec-x86_64-x64-vc14-Release.lib
                pjmedia-videodev-x86_64-x64-vc14-Release.lib
                pjmedia-x86_64-x64-vc14-Release.lib

                # PJNATH - Release版本
                pjnath-x86_64-x64-vc14-Release.lib

                # Third party - Release版本
                libbaseclasses-x86_64-x64-vc14-Release.lib
                libg7221codec-x86_64-x64-vc14-Release.lib
                libgsmcodec-x86_64-x64-vc14-Release.lib
                libilbccodec-x86_64-x64-vc14-Release.lib
                libmilenage-x86_64-x64-vc14-Release.lib
                libresample-x86_64-x64-vc14-Release.lib
                libspeex-x86_64-x64-vc14-Release.lib
                libsrtp-x86_64-x64-vc14-Release.lib
                libwebrtc-x86_64-x64-vc14-Release.lib
                libyuv-x86_64-x64-vc14-Release.lib

                # JRTPLIB - Release版本
                jrtplib.lib

                D:/third_party_libs/JThread/build/src/Release/jthread.lib

                # System
                ws2_32.lib

                # openssl - Release版本
                D:/third_party_libs/OpenSSL-Win64_/lib/VC/x64/MT/libcrypto.lib
                D:/third_party_libs/OpenSSL-Win64_/lib/VC/x64/MT/libssl.lib

                opus.lib
        )
    endif()
else()
    add_subdirectory(third_party/spdlog)
    add_subdirectory(third_party/jsoncpp)
    add_subdirectory(voip)
endif()

if(FEATURE STREQUAL "REMINDER")
    message(">>> target compile definition REMINDER")
    target_compile_definitions(${PROJECT_NAME} PRIVATE REMINDER)
elseif(FEATURE STREQUAL "ROBOT")
    message(">>> target compile definition ROBOT")
    target_compile_definitions(${PROJECT_NAME} PRIVATE ROBOT)
endif()

if(VOIP_SSL)
    message(">>> target compile definition VOIP_SSL")
    target_compile_definitions(${PROJECT_NAME} PRIVATE VOIP_SSL)
endif()