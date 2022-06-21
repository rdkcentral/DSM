cmake_minimum_required(VERSION 3.13)

project(dsm_core)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
message("[./] CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}")
message("[./] CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
include(src/CMakeLists.txt)
# add_subdirectory("src")
message("./ Target Build directory [CMAKE_BINARY_DIR]: ${CMAKE_BINARY_DIR}")



# set(ARCHIVE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/arch")
# set (PROJECT_SOURCE_DIR "../src")
