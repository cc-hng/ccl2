cmake_minimum_required(VERSION 3.14...3.22)

project(ccl2_test CXX)
set(LIB_NAME ccl2)

# ---- Options ----
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----
include(../cmake/common.cmake)
cxx_20()

find_package(Threads REQUIRED)
add_package("gh:google/googletest@1.12.0")

if(TEST_INSTALLED_VERSION)
  find_package(${LIB_NAME} REQUIRED)
else()
  add_local_package(
    NAME ccl2
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
    OPTIONS "CCL2_WITH_YYJSON ON" "CCL2_WITH_COROUTINES ON" "CCL2_WITH_HTTP ON"
  )
endif()

# ---- Create binary ----
file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(${PROJECT_NAME} ${LIB_NAME}::${LIB_NAME} gtest_main Threads::Threads)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(${LIB_NAME} PUBLIC -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(${LIB_NAME} PUBLIC /W4 /WX)
  endif()
endif()

# ---- Add GreeterTests ----
enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
