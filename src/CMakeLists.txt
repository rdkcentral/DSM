
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

project(dsm_core)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
message("[./src/] CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
message("[./src/] CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

option(ENABLE_RBUS_PROVIDER "Toggles the rbus provider to be built as part of DSM, requires librbusprovider and rbus to be installed" OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Werror -pthread")
message("[./src/] DSM g++ ${CMAKE_CXX_FLAGS}")

set(UTILS_SRC "${CMAKE_SOURCE_DIR}/utils/socket-server.cpp" 
              "${CMAKE_SOURCE_DIR}/utils/thread_worker.cpp" 
              "${CMAKE_SOURCE_DIR}/utils/command-context.cpp"
              "${CMAKE_SOURCE_DIR}/utils/execute-command.cpp"
              "${CMAKE_SOURCE_DIR}/utils/uuid_generator.cpp"
              "${CMAKE_SOURCE_DIR}/utils/file-system.cpp"
              )
set(DSM_SRC 
            "${CMAKE_SOURCE_DIR}/dsm-controller.cpp" 
            "${CMAKE_SOURCE_DIR}/datamodel/deployment-unit.cpp" 
            "${CMAKE_SOURCE_DIR}/datamodel/execution-environment.cpp" 
            "${CMAKE_SOURCE_DIR}/datamodel/execution-unit.cpp" 
            "${CMAKE_SOURCE_DIR}/datamodel/bridge/packager.cpp" 
            "${CMAKE_SOURCE_DIR}/datamodel/adapters/packager-adapter.cpp" 
            "${CMAKE_SOURCE_DIR}/datamodel/bridge/container-runtime.cpp"
            "${CMAKE_SOURCE_DIR}/datamodel/adapters/container-runtime-adapter.cpp"
            )

if (ENABLE_RBUS_PROVIDER)
    set(RBUS_SRC
                "${CMAKE_SOURCE_DIR}/rbus/dsm_rbus_provider.cpp"
                )

    include_directories(SYSTEM  /usr/include/rbus /usr/include/rbus_provider)
    add_definitions(-DENABLE_RBUS_PROVIDER)
endif(ENABLE_RBUS_PROVIDER)



add_executable(dsm "${CMAKE_SOURCE_DIR}/main.cpp" "${DSM_SRC}" "${UTILS_SRC}" "${RBUS_SRC}")

if (ENABLE_RBUS_PROVIDER)
    target_link_libraries(dsm rbus rbusprovider)
endif(ENABLE_RBUS_PROVIDER)   

add_executable(dsmcli "${CMAKE_SOURCE_DIR}/dsm-cli.cpp")

install(TARGETS dsm RUNTIME DESTINATION bin)
install(TARGETS dsmcli RUNTIME DESTINATION bin)

