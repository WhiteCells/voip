cmake_minimum_required(VERSION 3.10)

project(voip LANGUAGES CXX VERSION 1.0)

configure_file(${CMAKE_SOURCE_DIR}/.env ${CMAKE_BINARY_DIR}/voip/.env COPYONLY)

add_subdirectory(third_party/spdlog)
add_subdirectory(third_party/jsoncpp)
add_subdirectory(voip)
