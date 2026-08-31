cmake_minimum_required(VERSION 3.14)
project(mini_redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compiler warning configurations
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include_directories(include)

# Core static library
add_library(mini_redis_lib
    src/engine.cpp
    src/fen_compressor.cpp
    src/parser.cpp
    src/repl.cpp
    src/server.cpp
    src/skiplist.cpp
)

if(WIN32)
    target_link_libraries(mini_redis_lib PRIVATE ws2_32)
endif()

# Server Executable
add_executable(mini_redis_cli
    src/main.cpp
)
target_link_libraries(mini_redis_cli PRIVATE mini_redis_lib)

# Benchmark Executable
add_executable(mini_redis_benchmark
    src/benchmark.cpp
)
if(WIN32)
    target_link_libraries(mini_redis_benchmark PRIVATE ws2_32)
endif()
