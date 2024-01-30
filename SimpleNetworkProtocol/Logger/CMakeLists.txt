cmake_minimum_required(VERSION 3.16)

project(Logger)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

include_directories(${INCLUDE_DIRS})

add_library(Logger INTERFACE)

target_include_directories(Logger
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)
