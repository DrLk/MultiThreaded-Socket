cmake_minimum_required(VERSION 3.16)

project(Packet)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET LockedList)
    add_subdirectory(../LockedList LockdList)
endif()

set(INCLUDE_DIRS include)


file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)

include_directories(${INCLUDE_DIRS})

add_library(Packet STATIC ${SRC_FILES})
target_link_libraries(Packet PUBLIC LockedList)

target_include_directories(Packet
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
