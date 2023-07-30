cmake_minimum_required(VERSION 3.16)

project(Queue)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

if (NOT TARGET LockedList)
  add_subdirectory(../LockedList LockedList)
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp include/*.hpp)

include_directories(${INCLUDE_DIRS})

add_library(Queue STATIC ${SRC_FILES})

target_include_directories(Queue
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(WIN32)
  target_link_libraries(Queue PRIVATE wsock32 ws2_32 LockedList)
else()
  target_link_libraries(Queue PRIVATE pthread LockedList)
endif()
