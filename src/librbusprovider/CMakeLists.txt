
# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2022 Consult Red
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.13)

project(librbusprovider)
include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
message("[./src/] CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
message("[./src/] CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

option(ENABLE_STATIC_ANALYSIS "Run static analysis" OFF)

if (ENABLE_STATIC_ANALYSIS)
    set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=*,clang-analyzer-*,-readability-*,-fuchsia-*,-google-*,-modernize-use-trailing-return-type,-llvm-include-order)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pthread ")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Werror -Wshadow -Wnon-virtual-dtor -Wconversion -Wduplicated-cond -Wduplicated-branches  -Wlogical-op -Wnull-dereference -Wuseless-cast -Wformat=2 -pthread ") 
endif(ENABLE_STATIC_ANALYSIS)

set (CMAKE_CXX_STANDARD 17)

FILE(GLOB SRC "./*.cpp")
FILE(GLOB HEADERS "./*.h")


add_library(rbusprovider STATIC "${SRC}")
set_target_properties(  rbusprovider PROPERTIES PREFIX ""
                        rbusprovider PROPERTIES SOVERSION 1
                        rbusprovider PROPERTIES PUBLIC_HEADER "${HEADERS}"
                     )
         
include_directories(SYSTEM  /usr/include/rbus)
target_link_libraries(rbusprovider rbus)

INSTALL(TARGETS rbusprovider
        DESTINATION ${libdir}
        )