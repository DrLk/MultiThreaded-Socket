cmake_minimum_required(VERSION 3.25)

project(SimpleNetworkProtocolLib)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT TARGET Packet)
    add_subdirectory(../Packet Packet)
endif()
if (NOT TARGET Logger)
add_subdirectory(../Logger Logger)
endif()
if (NOT TARGET Memory)
add_subdirectory(../Memory Memory)
endif()
if (NOT TARGET LockedList)
    add_subdirectory(../LockedList LockdList)
endif()
if (NOT TARGET Tracy)
    add_subdirectory(../external/tracy Tracy EXCLUDE_FROM_ALL SYSTEM)
endif()

set(COMMON_INCLUDES include)

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)


include_directories(${COMMON_INCLUDES})

add_library(SimpleNetworkProtocolLib ${SRC_FILES})
target_link_libraries(SimpleNetworkProtocolLib PUBLIC Packet LockedList Logger Memory Tracy)
if(LINUX)
  target_link_libraries(SimpleNetworkProtocolLib PUBLIC Protocol)
endif()

if(WIN32)
  target_link_libraries(SimpleNetworkProtocolLib PRIVATE wsock32 ws2_32 )
else()
  target_link_libraries(SimpleNetworkProtocolLib PRIVATE pthread)
endif()

target_include_directories(Memory
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(ENABLE_PRECOMPILE_HEADERS)
  target_precompile_headers(SimpleNetworkProtocolLib PRIVATE ../pch.hpp)
endif()
