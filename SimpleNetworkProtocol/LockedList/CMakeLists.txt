cmake_minimum_required(VERSION 3.16)

project(LockedList)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

include_directories(${INCLUDE_DIRS})

add_library(LockedList INTERFACE )

target_include_directories(LockedList
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)