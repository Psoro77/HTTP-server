cmake_minimum_required(VERSION 3.10)
project(HighPerformanceHttpServer VERSION 1.0.0 LANGUAGES CXX)

# C++17 configuration
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compilation options for performance
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -mtune=native -DNDEBUG")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)

# Source files
set(SOURCES
    src/main.cpp
    src/ThreadPool.cpp
    src/Connection.cpp
    src/HttpRequest.cpp
    src/HttpResponse.cpp
    src/HttpServer.cpp
)

set(HEADERS
    src/ThreadPool.h
    src/Connection.h
    src/HttpRequest.h
    src/HttpResponse.h
    src/HttpServer.h
)

# Executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Specific compilation options
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -pthread
)

target_link_options(${PROJECT_NAME} PRIVATE -pthread)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
