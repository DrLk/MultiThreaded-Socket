cmake_minimum_required(VERSION 3.16)

project(FileSystem)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include include/FileCache)

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

include_directories(${INCLUDE_DIRS})

add_library(FileSystem STATIC ${SRC_FILES})

target_link_libraries(FileSystem PRIVATE Packet Logger Thread Tracy fuse3 pthread)

target_include_directories(FileSystem
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(FileSystem PUBLIC
    FUSE_USE_VERSION=35
)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(FileSystem PRIVATE ../pch.hpp)
endif()
