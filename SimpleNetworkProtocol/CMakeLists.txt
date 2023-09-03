cmake_minimum_required(VERSION 3.16)

project(SimpleNetworkProtocol)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(Queue)
if (NOT TARGET LockedList)
  add_subdirectory(LockedList)
endif()

set(COMMON_INCLUDES include)

file(GLOB_RECURSE SRC_FILES src/*.cpp include/*.hpp)

include_directories(${COMMON_INCLUDES})

add_library(SimpleNetworkProtocolLib ${SRC_FILES})
target_link_libraries(SimpleNetworkProtocolLib Queue LockedList)

if(WIN32)
  target_link_libraries(SimpleNetworkProtocolLib wsock32 ws2_32 )
else()
  target_link_libraries(SimpleNetworkProtocolLib pthread)
endif()

target_precompile_headers(SimpleNetworkProtocolLib PRIVATE include/pch.hpp)

add_executable(SimpleNetworkProtocol main.cpp)

target_link_libraries(SimpleNetworkProtocol SimpleNetworkProtocolLib)


##############
# Unit Tests
##############

add_subdirectory(googletest)

enable_testing()

file(GLOB_RECURSE TEST_SRC_FILES test/*.cpp )

add_executable(
  SimpleNetworkProtocolTest
  ${TEST_SRC_FILES}
)
target_link_libraries(
  SimpleNetworkProtocolTest
  SimpleNetworkProtocolLib
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(SimpleNetworkProtocolTest)
