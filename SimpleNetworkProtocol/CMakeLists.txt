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

if (NOT TARGET FileSystem)
  add_subdirectory(FileSystem)
endif()
if (NOT TARGET Protocol)
  add_subdirectory(Protocol)
endif()
if (NOT TARGET SimpleNetworkProtocolLib)
  add_subdirectory(SimpleNetworkProtocolLib)
endif()

add_executable(SimpleNetworkProtocol main.cpp)

target_link_libraries(SimpleNetworkProtocol PRIVATE SimpleNetworkProtocolLib FileSystem)
target_link_options(SimpleNetworkProtocol PRIVATE -static-libgcc -static-libstdc++)

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(SimpleNetworkProtocol PRIVATE pch.hpp)
endif()


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
    target_precompile_headers(SimpleNetworkProtocolTest PRIVATE pch.hpp)
endif()

# include(GoogleTest)
# gtest_discover_tests(SimpleNetworkProtocolTest)
endif()
