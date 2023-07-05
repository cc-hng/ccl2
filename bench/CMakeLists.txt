# for `FetchContent'
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project(ccl_bench LANGUAGES CXX)

# --- Import tools ----
include(../cmake/common.cmake)
cxx_20()

add_package(
  NAME benchmark
  GITHUB_REPOSITORY google/benchmark
  VERSION 1.6.1
  OPTIONS "BENCHMARK_ENABLE_TESTING OFF"
)

add_local_package(
  NAME ccl2
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
  OPTIONS "CCL2_WITH_YYJSON ON" "CCL2_WITH_COROUTINES ON"
)

# ---- Create standalone executable ----
file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(${PROJECT_NAME} ccl2::ccl2 benchmark)
