cmake_minimum_required(VERSION 3.16)

project(SimpleNetworkProtocol)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Queue)
if (NOT TARGET LockedList)
  add_subdirectory(LockedList)
endif()

set(INCLUDE_DIRS ${PROJECT_ROOT})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp)

include_directories(${INCLUDE_DIRS})

add_executable(SimpleNetworkProtocol ${SRC_FILES})

if(WIN32)
  target_link_libraries(SimpleNetworkProtocol wsock32 ws2_32 Queue LockedList)
else()
  target_link_libraries(SimpleNetworkProtocol pthread Queue LockedList)
endif()
