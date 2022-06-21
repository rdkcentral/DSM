cmake_minimum_required(VERSION 3.13)

project(dsm_core)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
message("[./src/] CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
message("[./src/] CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

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
add_executable(dsm "${CMAKE_SOURCE_DIR}/main.cpp" "${DSM_SRC}" "${UTILS_SRC}")
add_executable(dsmcli "${CMAKE_SOURCE_DIR}/dsm-cli.cpp")

install(TARGETS dsm RUNTIME DESTINATION bin)
install(TARGETS dsmcli RUNTIME DESTINATION bin)

