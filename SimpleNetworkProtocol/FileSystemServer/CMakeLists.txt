cmake_minimum_required(VERSION 3.16)

project(FileSystemServer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET FileSystemCommon)
    add_subdirectory(../FileSystemCommon FileSystemCommon)
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)

add_library(FileSystemServer STATIC ${SRC_FILES})

target_link_libraries(FileSystemServer PUBLIC FileSystemCommon)

target_include_directories(FileSystemServer
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(FileSystemServer PRIVATE ../pch.hpp)
endif()
