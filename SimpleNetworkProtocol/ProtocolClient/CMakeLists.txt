cmake_minimum_required(VERSION 3.16)

project(ProtocolClient)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET ProtocolCommon)
    add_subdirectory(../ProtocolCommon ProtocolCommon)
endif()
if (NOT TARGET FileSystemClient)
    add_subdirectory(../FileSystemClient FileSystemClient)
endif()

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/FileSystem/*.cpp
    src/FileSystem/FileCache/*.cpp
    include/*.hpp
    include/FileSystem/*.hpp
    include/FileSystem/FileCache/*.hpp)

add_library(ProtocolClient STATIC ${SRC_FILES})

target_link_libraries(ProtocolClient PUBLIC ProtocolCommon FileSystemClient)

target_include_directories(ProtocolClient
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/FileSystem
        ${PROJECT_SOURCE_DIR}/include/FileSystem/FileCache
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(ProtocolClient PRIVATE ../pch.hpp)
endif()
