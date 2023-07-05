# for `FetchContent'
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ccl2 VERSION 0.1.0)

# ---- options ----
option(CCL2_WITH_YYJSON "use yyjson" ON)
option(CCL2_WITH_FMT "use fmt instead of `printf/sprintf'" ON)
option(CCL2_WITH_COROUTINES "use c++20 coroutines" ON)
option(CCL2_WITH_HTTP "use http" OFF)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common.cmake)

# cmake-format: off
macro(set_cache arg1 arg2)
  set(${arg1} ${arg2} CACHE INTERNAL "")
endmacro(set_cache)
# cmake-format: on

# ---- set cxx standard (default use c++17) ----
if(CCL2_WITH_COROUTINES)
  cxx_20()
else()
  cxx_17()
endif()

# ---- third package ----
add_package("gh:TheLartians/PackageProject.cmake@1.10.0")

set_cache(BUILD_SHARED_LIBS OFF)
set_cache(GSL_INSTALL ON)
add_package("gh:microsoft/GSL@4.0.0")

# boost
set(Boost_DEBUG OFF)
set(Boost_USE_STATIC_LIBS ON)
set_cache(Boost_NO_WARN_NEW_VERSIONS ON)
find_package(Boost 1.72 REQUIRED)

find_package(Threads REQUIRED)

# fmt
if(CCL2_WITH_FMT)
  set_cache(FMT_INSTALL ON)
  add_package("gh:fmtlib/fmt#9.1.0")
  list(APPEND third_packages fmt)
  list(APPEND third_libs fmt::fmt)
endif()

if(CCL2_WITH_COROUTINES)
  add_package(GIT_REPOSITORY https://git.l2x.top/cc/std_coroutines.git VERSION 0.2.0)
  list(APPEND third_packages std_coroutines)
  list(APPEND third_libs std::coroutines)
endif()

# yyjson
if(CCL2_WITH_YYJSON)
  add_package("gh:ibireme/yyjson#0.6.0")
  list(APPEND third_packages yyjson)
  list(APPEND third_libs yyjson)
endif()

# ---- Add source files ----
# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/**.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# ---- Create library ----
if(CCL2_WITH_HTTP)
  add_library(${PROJECT_NAME} STATIC ${headers} ${sources})
  target_compile_definitions(
    ${PROJECT_NAME} PUBLIC $<$<BOOL:${CCL2_WITH_YYJSON}>:CCL2_USE_YYJSON>
                           $<$<BOOL:${CCL2_WITH_COROUTINES}>:CCL2_USE_COROUTINES>
  )
  target_link_libraries(
    ${PROJECT_NAME} PUBLIC Microsoft.GSL::GSL Boost::headers ${third_libs} Threads::Threads
  )
  target_include_directories(
    ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                           $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  )
else()
  add_library(${PROJECT_NAME} INTERFACE)
  target_compile_definitions(
    ${PROJECT_NAME} INTERFACE $<$<BOOL:${CCL2_WITH_YYJSON}>:CCL2_USE_YYJSON>
                              $<$<BOOL:${CCL2_WITH_COROUTINES}>:CCL2_USE_COROUTINES>
  )
  target_link_libraries(
    ${PROJECT_NAME} INTERFACE Microsoft.GSL::GSL Boost::headers ${third_libs} Threads::Threads
  )
  target_include_directories(
    ${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                              $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
  )
endif()

# ---- Create an installable target ----
packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_HEADER_PATTERN "*.h"
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${PROJECT_NAME}/version.h"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "${third_packages};Threads;Microsoft.GSL;Boost 1.7.2"
)
