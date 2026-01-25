cmake_minimum_required(VERSION 3.29)
project(lamar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/third_party/Lama/runtime)

add_subdirectory(${RUNTIME_DIR} ${CMAKE_BINARY_DIR}/third_party/Lama/runtime)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Lama/byterun ${CMAKE_BINARY_DIR}/third_party/Lama/byterun)

add_executable(lamar
        include/bytecode.hpp
        include/disassembler.hpp
        src/main.cpp
        src/loader.cpp
        src/bytecode.cpp
        src/disassembler.cpp)

target_include_directories(lamar PRIVATE include/ ${RUNTIME_DIR})
target_link_libraries(lamar PRIVATE runtime)
