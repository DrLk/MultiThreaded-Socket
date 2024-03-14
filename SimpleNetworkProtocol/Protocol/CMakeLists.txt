cmake_minimum_required(VERSION 3.16)

project(Protocol)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

if (NOT TARGET FileSystem)
    add_subdirectory(../FileSystem FileSystem)
endif()
if (NOT TARGET LockedList)
    add_subdirectory(../LockedList LockdList)
endif()
if (NOT TARGET SimpleNetworkProtocolLib)
    add_subdirectory(../SimpleNetworkProtocolLib SimpleNetworkProtocolLib)
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)

include_directories(${INCLUDE_DIRS})

add_library(Protocol STATIC ${SRC_FILES})

target_include_directories(Protocol
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(Protocol PRIVATE FileSystem LockedList SimpleNetworkProtocolLib)
if(WIN32)
    target_link_libraries(Protocol PRIVATE wsock32 ws2_32)
else()
  target_link_libraries(Protocol PRIVATE pthread)
endif()
