cmake_minimum_required(VERSION 3.16)

project(LockedList)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

include_directories(${INCLUDE_DIRS})

if (NOT TARGET Thread)
    add_subdirectory(../Thread Thread)
endif()

add_library(LockedList INTERFACE )

target_link_libraries(LockedList INTERFACE Thread)

target_include_directories(LockedList
    INTERFACE
        ${PROJECT_SOURCE_DIR}/include
)
