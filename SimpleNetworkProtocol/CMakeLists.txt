cmake_minimum_required(VERSION 3.25)

option(ENABLE_PRECOMPILE_HEADERS "Enable Precompile Headers" ON)
option(ENABLE_TRACY "Enable Tracy" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer for tests" ON)
option(ENABLE_ASAN "Enable Address Sanitizer for tests" OFF)
option(ENABLE_UBSAN "Enable Undefined Behavior Sanitizer for tests" ON)

project(SimpleNetworkProtocol)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_TSAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(TSAN_ACTIVE TRUE)
else()
  set(TSAN_ACTIVE FALSE)
endif()

if(ENABLE_ASAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(ASAN_ACTIVE TRUE)
else()
  set(ASAN_ACTIVE FALSE)
endif()

if(ENABLE_UBSAN AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(UBSAN_ACTIVE TRUE)
else()
  set(UBSAN_ACTIVE FALSE)
endif()

if(TSAN_ACTIVE AND ASAN_ACTIVE)
  message(FATAL_ERROR "ENABLE_TSAN and ENABLE_ASAN are mutually exclusive")
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo" AND NOT TSAN_ACTIVE AND NOT ASAN_ACTIVE AND NOT UBSAN_ACTIVE)
  add_compile_options(-flto=thin)
  add_link_options(-flto=thin)
  if(NOT CMAKE_EXE_LINKER_FLAGS MATCHES "fuse-ld")
    add_link_options(-fuse-ld=lld)
  endif()
endif()


if(MSVC)
  add_compile_options(/MP)
endif()

if(TSAN_ACTIVE)
  add_compile_options(-fsanitize=thread)
  add_link_options(-fsanitize=thread)
endif()

if(ASAN_ACTIVE)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()

if(UBSAN_ACTIVE)
  add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=undefined)
endif()

if(LINUX)
if (NOT TARGET FileSystemCommon)
  add_subdirectory(FileSystemCommon)
endif()
if (NOT TARGET FileSystemServer)
  add_subdirectory(FileSystemServer)
endif()
if (NOT TARGET FileSystemClient)
  add_subdirectory(FileSystemClient)
endif()
if (NOT TARGET ProtocolCommon)
  add_subdirectory(ProtocolCommon)
endif()
if (NOT TARGET ProtocolServer)
  add_subdirectory(ProtocolServer)
endif()
if (NOT TARGET ProtocolClient)
  add_subdirectory(ProtocolClient)
endif()
if (NOT TARGET SimpleNetworkProtocolLib)
  add_subdirectory(SimpleNetworkProtocolLib)
endif()
endif()

if(LINUX)
  # The server binary links only common+server archives — there is no
  # libfuse3 dependency on this side. The client binary links common+client
  # archives, which pull in libfuse3 via FileSystemClient.
  add_executable(SimpleNetworkProtocolServer main_server.cpp)
  target_link_libraries(SimpleNetworkProtocolServer PRIVATE
    SimpleNetworkProtocolLib
    "$<LINK_GROUP:RESCAN,ProtocolCommon,ProtocolServer,FileSystemCommon,FileSystemServer>")
  if(NOT TSAN_ACTIVE AND NOT ASAN_ACTIVE AND NOT UBSAN_ACTIVE)
    target_link_options(SimpleNetworkProtocolServer PRIVATE -static-libgcc -static-libstdc++)
  endif()
  if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(SimpleNetworkProtocolServer PRIVATE pch.hpp)
  endif()

  add_executable(SimpleNetworkProtocolClient main_client.cpp)
  target_link_libraries(SimpleNetworkProtocolClient PRIVATE
    SimpleNetworkProtocolLib
    "$<LINK_GROUP:RESCAN,ProtocolCommon,ProtocolClient,FileSystemCommon,FileSystemClient>")
  if(NOT TSAN_ACTIVE AND NOT ASAN_ACTIVE AND NOT UBSAN_ACTIVE)
    target_link_options(SimpleNetworkProtocolClient PRIVATE -static-libgcc -static-libstdc++)
  endif()
  if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(SimpleNetworkProtocolClient PRIVATE pch.hpp)
  endif()
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
  if(LINUX)
  target_link_libraries(SimpleNetworkProtocolTest PRIVATE
    "$<LINK_GROUP:RESCAN,ProtocolCommon,ProtocolServer,ProtocolClient,FileSystemCommon,FileSystemServer,FileSystemClient>")
endif()
  if(NOT TSAN_ACTIVE AND NOT ASAN_ACTIVE AND NOT UBSAN_ACTIVE)
    target_link_options(SimpleNetworkProtocolTest
        PRIVATE
        -static-libgcc
        -static-libstdc++)
  endif()

  target_compile_definitions(SimpleNetworkProtocolTest PRIVATE
      UNIT_TESTING=1
      )

if(ENABLE_PRECOMPILE_HEADERS)
    target_precompile_headers(SimpleNetworkProtocolTest PRIVATE pch.hpp)
endif()

# include(GoogleTest)
# gtest_discover_tests(SimpleNetworkProtocolTest)
endif()
