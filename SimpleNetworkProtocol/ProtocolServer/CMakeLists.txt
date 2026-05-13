cmake_minimum_required(VERSION 3.16)

project(ProtocolServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET ProtocolCommon)
    add_subdirectory(../ProtocolCommon ProtocolCommon)
endif()
if (NOT TARGET FileSystemServer)
    add_subdirectory(../FileSystemServer FileSystemServer)
endif()

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    src/FileSystem/*.cpp
    src/FileSystem/FileCache/*.cpp
    include/*.hpp
    include/FileSystem/*.hpp
    include/FileSystem/FileCache/*.hpp)

add_library(ProtocolServer STATIC ${SRC_FILES})

target_link_libraries(ProtocolServer PUBLIC ProtocolCommon FileSystemServer)

target_include_directories(ProtocolServer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/FileSystem
        ${PROJECT_SOURCE_DIR}/include/FileSystem/FileCache
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(ProtocolServer PRIVATE ../pch.hpp)
endif()
