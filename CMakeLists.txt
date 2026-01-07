cmake_minimum_required(VERSION 3.29)
project(lamar)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_OSX_ARCHITECTURES "x86_64")

include(ExternalProject)
find_program(MAKE_EXECUTABLE NAMES gmake make mingw32-make REQUIRED)

#set(RUNTIME_LIB ${CMAKE_INSTALL_FULL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}runtime${CMAKE_STATIC_LIBRARY_SUFFIX})
#
#
#ExternalProject_Add(
#        runtime
#        SOURCE_DIR "third_party/Lama/"
#        PREFIX "${CMAKE_BINARY_DIR}/Lama/runtime"
#        BUILD_COMMAND ${MAKE_EXECUTABLE} -j -C third_party/Lama/runtime
#        BUILD_BYPRODUCTS ${RUNTIME_LIB}
#)

set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/third_party/Lama/runtime)

#find_package(runtime REQUIRED)

include_directories(${RUNTIME_DIR})
include_directories(include/)
add_executable(lamar include/bytecode.hpp src/main.cpp
        src/loader.cpp
        src/bytecode.cpp)

execute_process(
        COMMAND ${MAKE_EXECUTABLE} -j -C ${RUNTIME_DIR}
)

target_link_libraries(lamar ${RUNTIME_DIR}/runtime.a)

