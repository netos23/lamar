cmake_minimum_required(VERSION 3.29)
project(lamar CXX C ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "x86_64")
set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/third_party/Lama/runtime)

add_subdirectory(${RUNTIME_DIR} ${CMAKE_BINARY_DIR}/third_party/Lama/runtime)

add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/Lama/byterun ${CMAKE_BINARY_DIR}/third_party/Lama/byterun)

set(COMMON_SOURCES
        include/bytecode.hpp
        include/diagnostics.hpp
        include/disassembler.hpp
        include/utils.hpp

        include/loader.hpp
        include/runtime.hpp

        src/common/bytecode.cpp
        src/common/diagnostics.cpp
        src/disassembler/disassembler.cpp

        src/loader/loader.cpp
        src/common/utils.cpp

)
set(LAMAR_SOURCES
        include/interpreter.hpp
        include/value.hpp
        include/frame.hpp
        include/verifier.hpp

        src/interpreter/interpreter.cpp
        src/interpreter/value.cpp
        src/interpreter/frame.cpp
        src/verifier/verifier.cpp

        src/driver.cpp
)
set(ANALYZER_SOURCES
        include/idiom_analyzer.hpp
        src/idiom_analyzer/idiom_analyzer.cpp
        src/analyzer.cpp
)
set(DISASSEMBLER_SOURCES
        src/disassembler.cpp
)

add_executable(lamar
        ${COMMON_SOURCES}
        ${LAMAR_SOURCES}
)

# Disable sanitizers on linux
if (CMAKE_BUILD_TYPE MATCHES "Debug" AND CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_link_options(lamar
            BEFORE PRIVATE -fsanitize=undefined -fsanitize=address
    )
else ()
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    endif ()
endif ()

add_compile_definitions(ENABLE_VERIFICATION MESHURE_TIME)

target_include_directories(lamar PRIVATE include/ ${RUNTIME_DIR})
target_link_libraries(lamar PRIVATE runtime)


add_executable(lamar_analyzer
        ${COMMON_SOURCES}
        ${ANALYZER_SOURCES}
)
target_include_directories(lamar_analyzer PRIVATE include/ ${RUNTIME_DIR})
target_link_libraries(lamar_analyzer PRIVATE runtime)

add_executable(lamar_disassembler
        ${COMMON_SOURCES}
        ${DISASSEMBLER_SOURCES}
)
target_include_directories(lamar_disassembler PRIVATE include/ ${RUNTIME_DIR})
target_link_libraries(lamar_disassembler PRIVATE runtime)
