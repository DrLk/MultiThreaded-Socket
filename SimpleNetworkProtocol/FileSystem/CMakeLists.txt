cmake_minimum_required(VERSION 3.16)

project(FileSystem)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

if (NOT TARGET Logger)
    add_subdirectory(../Logger Logger)
endif()

if (NOT TARGET Thread)
	add_subdirectory(../Thread Thread)
endif()

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp include/*.hpp)

include_directories(${INCLUDE_DIRS})

add_library(FileSystem STATIC ${SRC_FILES})

target_link_libraries(FileSystem PUBLIC Logger Thread)

target_include_directories(FileSystem
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)
