cmake_minimum_required(VERSION 3.29)
project(lamar CXX C ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/third_party/Lama/runtime)

add_subdirectory(${RUNTIME_DIR} ${CMAKE_BINARY_DIR}/third_party/Lama/runtime)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Lama/byterun ${CMAKE_BINARY_DIR}/third_party/Lama/byterun)

add_executable(lamar

        include/bytecode.hpp
        include/disassembler.hpp
        include/loader.hpp
        include/interpreter.hpp
        include/runtime.hpp
        include/value.hpp

        src/common/bytecode.cpp
        src/disassembler/disassembler.cpp
        src/interpreter/interpreter.cpp
        src/interpreter/value.cpp
        src/loader/loader.cpp

        src/main.cpp
        include/frame.hpp
        src/interpreter/frame.cpp
)

if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(
            CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -Werror -fsanitize=undefined -fsanitize=address"
    )
    set(
            CMAKE_CXX_FLAGS
            "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address"
    )
    target_link_options(lamar
            BEFORE PRIVATE -fsanitize=undefined -fsanitize=address
    )
endif ()


target_include_directories(lamar PRIVATE include/ ${RUNTIME_DIR})
target_link_libraries(lamar PRIVATE runtime)

