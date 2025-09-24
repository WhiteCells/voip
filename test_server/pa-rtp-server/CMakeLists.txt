cmake_minimum_required(VERSION 3.10)

project(pa-rtp-server LANGUAGES CXX VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    if (MSVC)
        add_compile_options(/bigobj)
        add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
    endif()

    set(BOOST_ROOT           "D:/third_party_libs/boost")
    set(BOOST_LIBRARYDIR     "${BOOST_ROOT}/stage/lib")

    set(SPDLOG_INCLUDE_DIR   "D:/third_party_libs/spdlog/include")
    set(SPDLOG_LIBRARY_DIR   "D:/third_party_libs/spdlog/build/Release")

    set(JSONCPP_INCLUDE_DIR  "D:/third_party_libs/jsoncpp/include")
    set(JSONCPP_LIBRARY_DIR  "D:/third_party_libs/jsoncpp/build/lib/Release")

    set(JRTPLIB_INCLUDE_DIR  "D:/third_party_libs/JRTPLIB/src")
    set(JRTPLIB_LIBRARY_DIR  "D:/third_party_libs/JRTPLIB/build/src/Release")

    set(PORTAUDIO_INCLUDE_DIR "D:/third_party_libs/portaudio/include")
    set(PORTAUDIO_LIBRARY_DIR "D:/third_party_libs/portaudio/build/Release")

    set(OPUSLIB_INCLUDE_DIR "D:/third_party_libs/opus-1.5.2/include")
    set(OPUSLIB_LIBRARY_DIR "D:/third_party_libs/opus-1.5.2/build_release/Release")

    set(PJPROJECT_DIR        "D:/third_party_libs/pjproject")
    set(PJSIP_INCLUDE_DIRS
            "${PJPROJECT_DIR}/pjsip/include"
            "${PJPROJECT_DIR}/pjlib/include"
            "${PJPROJECT_DIR}/pjlib-util/include"
            "${PJPROJECT_DIR}/pjmedia/include"
            "${PJPROJECT_DIR}/pjnath/include"
    )

    set(PJSIP_LIBRARY_DIR "${PJPROJECT_DIR}/pjsip/lib")
    set(PJLIB_LIBRARY_DIR "${PJPROJECT_DIR}/pjlib/lib")
    set(PJLIB_UTIL_LIBRARY_DIR "${PJPROJECT_DIR}/pjlib-util/lib")
    set(PJMEDIA_LIBRARY_DIR "${PJPROJECT_DIR}/pjmedia/lib")
    set(PJNATH_LIBRARY_DIR "${PJPROJECT_DIR}/pjnath/lib")
    set(THIRD_PARTY_LIB_DIR "${PJPROJECT_DIR}/third_party/lib")

    set(VOIP_SRC
            src/main.cc
    )

    set(SRC ${VOIP_SRC})

    if(POLICY CMP0167)
        cmake_policy(SET CMP0167 NEW)
    endif()

    find_package(Boost REQUIRED COMPONENTS system filesystem)
    add_executable(${PROJECT_NAME} ${SRC})

    target_include_directories(${PROJECT_NAME} PRIVATE
            ${Boost_INCLUDE_DIRS}
            ${SPDLOG_INCLUDE_DIR}
            ${JSONCPP_INCLUDE_DIR}
            ${PJSIP_INCLUDE_DIRS}
            ${JRTPLIB_INCLUDE_DIR}
            D:/third_party_libs/JRTPLIB/build/src
            D:/third_party_libs/JRTPLIB/src
            D:/third_party_libs/JThread/src
            ${PORTAUDIO_INCLUDE_DIR}
            ${OPUSLIB_INCLUDE_DIR}
    )

    target_link_directories(${PROJECT_NAME} PRIVATE
            ${BOOST_LIBRARYDIR}
            ${SPDLOG_LIBRARY_DIR}
            ${JSONCPP_LIBRARY_DIR}
            ${PJSIP_LIBRARY_DIR}
            ${PJLIB_LIBRARY_DIR}
            ${PJLIB_UTIL_LIBRARY_DIR}
            ${PJMEDIA_LIBRARY_DIR}
            ${PJNATH_LIBRARY_DIR}
            ${THIRD_PARTY_LIB_DIR}
            ${JRTPLIB_LIBRARY_DIR}
            ${PORTAUDIO_LIBRARY_DIR}
            ${OPUSLIB_LIBRARY_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
            # Boost
            ${Boost_LIBRARIES}

            # JsonCpp
            jsoncpp_static.lib

            # PJSIP
            pjsip-core-x86_64-x64-vc14-Release.lib
            pjsip-simple-x86_64-x64-vc14-Release.lib
            pjsip-ua-x86_64-x64-vc14-Release.lib
            pjsua-lib-x86_64-x64-vc14-Release.lib
            pjsua2-lib-x86_64-x64-vc14-Release.lib

            # PJLIB
            pjlib-x86_64-x64-vc14-Release.lib
            pjlib-util-x86_64-x64-vc14-Release.lib

            # PJMEDIA
            pjmedia-audiodev-x86_64-x64-vc14-Release.lib
            pjmedia-codec-x86_64-x64-vc14-Release.lib
            pjmedia-videodev-x86_64-x64-vc14-Release.lib
            pjmedia-x86_64-x64-vc14-Release.lib

            # PJNATH
            pjnath-x86_64-x64-vc14-Release.lib

            # Third party
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

            # JRTPLIB
            jrtplib.lib

            D:/third_party_libs/JThread/build/src/Release/jthread.lib
            # System
            ws2_32.lib

            # openssl
            D:/vcpkg/packages/openssl_x64-windows/lib/libcrypto.lib
            D:/vcpkg/packages/openssl_x64-windows/lib/libssl.lib

            portaudio.lib

            opus.lib

    )
else()
    add_subdirectory(src)
endif ()

