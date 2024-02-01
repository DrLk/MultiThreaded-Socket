cmake_minimum_required(VERSION 3.16)

option(ENABLE_PRECOMPILE_HEADERS "Enable Precompile Headers" ON)
option(ENABLE_TRACY "Enable Tracy" OFF)

project(SimpleNetworkProtocol)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/MP)
endif()

add_subdirectory(Logger)
add_subdirectory(Memory)
add_subdirectory(Queue)
add_subdirectory(FileSystem)
if (NOT TARGET LockedList)
  add_subdirectory(LockedList)
endif()
add_subdirectory(external/tracy EXCLUDDE_FROM_ALL SYSTEM)


set(COMMON_INCLUDES include)

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)


include_directories(${COMMON_INCLUDES})

add_library(SimpleNetworkProtocolLib ${SRC_FILES})
target_link_libraries(SimpleNetworkProtocolLib PUBLIC Queue LockedList Logger Memory Tracy)

if(WIN32)
  target_link_libraries(SimpleNetworkProtocolLib PRIVATE wsock32 ws2_32 )
else()
  target_link_libraries(SimpleNetworkProtocolLib PRIVATE pthread)
endif()

if(ENABLE_PRECOMPILE_HEADERS)
  target_precompile_headers(SimpleNetworkProtocolLib PRIVATE include/pch.hpp)
endif()

add_executable(SimpleNetworkProtocol main.cpp)

target_link_libraries(SimpleNetworkProtocol PRIVATE SimpleNetworkProtocolLib FileSystem)
target_link_options(SimpleNetworkProtocol PRIVATE -static-libgcc -static-libstdc++)


##############
# Unit Tests
##############
option(ENABLE_TESTS "Enable Unit Tests" ON)

if(ENABLE_TESTS)
  add_subdirectory(external/googletest EXCLUDE_FROM_ALL SYSTEM)

  enable_testing()

  file(GLOB_RECURSE TEST_SRC_FILES test/*.cpp )

  add_executable(
    SimpleNetworkProtocolTest
    ${TEST_SRC_FILES}
  )
  target_link_libraries(
    SimpleNetworkProtocolTest
    PRIVATE
    SimpleNetworkProtocolLib
    GTest::gtest_main
    GTest::gmock_main
  )
  target_link_options(SimpleNetworkProtocolTest
      PRIVATE
      -static-libgcc
      -static-libstdc++)

if(ENABLE_PRECOMPILE_HEADERS)
  target_precompile_headers(SimpleNetworkProtocolTest PRIVATE include/pch.hpp)
endif()

# include(GoogleTest)
# gtest_discover_tests(SimpleNetworkProtocolTest)
endif()
