
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

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
message("[./] CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}")
message("[./] CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
include(src/CMakeLists.txt)
message("./ Target Build directory [CMAKE_BINARY_DIR]: ${CMAKE_BINARY_DIR}")
