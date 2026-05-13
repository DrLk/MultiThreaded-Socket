cmake_minimum_required(VERSION 3.16)

project(FileSystemClient)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET FileSystemCommon)
    add_subdirectory(../FileSystemCommon FileSystemCommon)
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)

add_library(FileSystemClient STATIC ${SRC_FILES})

target_link_libraries(FileSystemClient PUBLIC FileSystemCommon fuse3)

target_compile_definitions(FileSystemClient PUBLIC
    FUSE_USE_VERSION=312
)

target_include_directories(FileSystemClient
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(FileSystemClient PRIVATE ../pch.hpp)
endif()
