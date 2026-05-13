cmake_minimum_required(VERSION 3.16)

project(ProtocolCommon)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET FileSystemCommon)
    add_subdirectory(../FileSystemCommon FileSystemCommon)
endif()
if (NOT TARGET LockedList)
    add_subdirectory(../LockedList LockedList)
endif()
if (NOT TARGET SimpleNetworkProtocolLib)
    add_subdirectory(../SimpleNetworkProtocolLib SimpleNetworkProtocolLib)
endif()

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/FileSystem/*.cpp
    src/FileSystem/FileCache/*.cpp
    include/*.hpp
    include/FileSystem/*.hpp
    include/FileSystem/FileCache/*.hpp)

add_library(ProtocolCommon STATIC ${SRC_FILES})

target_link_libraries(ProtocolCommon PUBLIC FileSystemCommon LockedList SimpleNetworkProtocolLib)
if(WIN32)
    target_link_libraries(ProtocolCommon PRIVATE wsock32 ws2_32)
else()
    target_link_libraries(ProtocolCommon PRIVATE pthread)
endif()

target_include_directories(ProtocolCommon
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/FileSystem
        ${PROJECT_SOURCE_DIR}/include/FileSystem/FileCache
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(ProtocolCommon PRIVATE ../pch.hpp)
endif()
