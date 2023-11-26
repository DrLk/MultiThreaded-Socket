cmake_minimum_required(VERSION 3.16)

project(Memory)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(INCLUDE_DIRS include)

if (NOT TARGET Logger)
    add_subdirectory(../Logger Logger)
endif()

add_subdirectory(../Thread Thread)

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.hpp include/*.hpp)

include_directories(${INCLUDE_DIRS})

add_library(Memory STATIC ${SRC_FILES})

target_link_libraries(Memory PUBLIC Logger Thread)

target_include_directories(Memory
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)