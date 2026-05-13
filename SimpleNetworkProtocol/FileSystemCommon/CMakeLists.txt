cmake_minimum_required(VERSION 3.16)

project(FileSystemCommon)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (NOT TARGET Packet)
    add_subdirectory(../Packet Packet)
endif()
if (NOT TARGET Logger)
    add_subdirectory(../Logger Logger)
endif()
if (NOT TARGET Thread)
    add_subdirectory(../Thread Thread)
endif()
if (NOT TARGET Tracy)
    add_subdirectory(../external/tracy Tracy EXCLUDE_FROM_ALL SYSTEM)
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp src/FileCache/*.cpp include/*.hpp include/FileCache/*.hpp)

add_library(FileSystemCommon STATIC ${SRC_FILES})

target_link_libraries(FileSystemCommon PUBLIC Packet Logger Thread Tracy pthread)

target_include_directories(FileSystemCommon
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include/FileCache
)

target_compile_definitions(FileSystemCommon PUBLIC
    FUSE_USE_VERSION=312
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(FileSystemCommon PRIVATE ../pch.hpp)
endif()
