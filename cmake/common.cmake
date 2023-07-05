# only activate tools for top level project
if(NOT PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  return()
endif()

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ##################################################################################################
# environment & utils
# ##################################################################################################
set(__DEBUG__ OFF)
if(NOT CMAKE_BUILD_TYPE OR (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  set(__DEBUG__ ON)
endif()

# cmake-format: off
# Get a value:
#   1. ${VAR}
#   2. $ENV{MY_${VAR}}
#   3. default var
function(_update_value VAR)
  set(extra_args ${ARGN})
  list(LENGTH extra_args extra_count)
  set(DEF "")
  if (${extra_count} GREATER 1)
    message(FATAL_ERROR "Too many args")
  elseif (${extra_count} EQUAL 1)
    list(GET extra_args 0 def_arg)
    set(DEF ${def_arg})
  endif()

  if(NOT ${VAR})
    if(DEFINED ENV{MY_${VAR}})
      set(${VAR} $ENV{MY_${VAR}})
    else()
      set(${VAR} ${DEF})
    endif()
  endif()

  set(null_list "x" "xOFF" "xNO")
  list(FIND null_list "x${${VAR}}" null_index)
  if (${null_index} EQUAL -1)
    set(${VAR} ${${VAR}} CACHE INTERNAL "")
  else()
    unset(${VAR})
  endif()
endfunction()
# cmake-format: on

_update_value(USE_SANITIZER "")
_update_value(USE_STATIC_ANALYZER OFF)

macro(_clear_variable)
  cmake_parse_arguments(CLEAR_VAR "" "DESTINATION;BACKUP;REPLACE" "" ${ARGN})
  set(${CLEAR_VAR_BACKUP} ${${CLEAR_VAR_DESTINATION}})
  set(${CLEAR_VAR_DESTINATION} ${CLEAR_VAR_REPLACE})
endmacro()

macro(_restore_variable)
  cmake_parse_arguments(CLEAR_VAR "" "DESTINATION;BACKUP" "" ${ARGN})
  set(${CLEAR_VAR_DESTINATION} ${${CLEAR_VAR_BACKUP}})
  unset(${CLEAR_VAR_BACKUP})
endmacro()

macro(my_info)
  message(STATUS "MY: " ${ARGN})
endmacro(my_info)

macro(add_package)
  if(${__DEBUG__} AND USE_STATIC_ANALYZER)
    _clear_variable(DESTINATION CMAKE_CXX_CLANG_TIDY BACKUP CMAKE_CXX_CLANG_TIDY_BKP)
    _clear_variable(
      DESTINATION CMAKE_CXX_INCLUDE_WHAT_YOU_USE BACKUP CMAKE_CXX_INCLUDE_WHAT_YOU_USE_BKP
    )
  endif()
  CPMAddPackage(${ARGN})
  if(${__DEBUG__} AND USE_STATIC_ANALYZER)
    _restore_variable(DESTINATION CMAKE_CXX_CLANG_TIDY BACKUP CMAKE_CXX_CLANG_TIDY_BKP)
    _restore_variable(
      DESTINATION CMAKE_CXX_INCLUDE_WHAT_YOU_USE BACKUP CMAKE_CXX_INCLUDE_WHAT_YOU_USE_BKP
    )
  endif()
endmacro(add_package)

macro(add_local_package)
  CPMAddPackage(${ARGN})
endmacro(add_local_package)

# Get all propreties that cmake supports
set(CMAKE_PROPERTY_LIST
    NAME
    TYPE
    SOURCE_DIR
    COMPILE_OPTIONS
    LINK_OPTIONS
    INCLUDE_DIRECTORIES
    INTERFACE_COMPILE_DEFINITIONS
    INTERFACE_INCLUDE_DIRECTORIES
    INTERFACE_LINK_LIBRARIES
    INTERFACE_SYSTEM_INCLUDE_DIRECTORIES
)

function(my_print_target_properties target)
  if(NOT TARGET ${target})
    message(STATUS "There is no target named '${target}'")
    return()
  endif()

  foreach(property ${CMAKE_PROPERTY_LIST})
    string(REPLACE "<CONFIG>" "DEBUG" property ${property})

    get_property(
      was_set
      TARGET ${target}
      PROPERTY ${property}
      SET
    )
    if(was_set)
      get_target_property(value ${target} ${property})
      message("${target}> ${property} = ${value}")
    endif()
  endforeach()
endfunction()

# --- output proj information ---
macro(my_print_base_info)
  my_info("******************************************************* ")
  my_info("PROJECT_NAME: \t${PROJECT_NAME}")
  my_info("PROJECT_DIR: \t${PROJECT_SOURCE_DIR}")
  my_info("OS: \t\t${CMAKE_SYSTEM}")
  my_info("ARCH: \t\t${CMAKE_SYSTEM_PROCESSOR}")
  my_info("BUILD_TYPE: \t${CMAKE_BUILD_TYPE}")
  my_info("CXX_STANDARD: \t${CMAKE_CXX_STANDARD}")
  my_info("CXX_FLAGS: \t${CMAKE_CXX_FLAGS}")
  my_info("Compiler: \t${CMAKE_CXX_COMPILER}")
  my_info("******************************************************* ")
endmacro()

# ##################################################################################################
# compile & link options
# ##################################################################################################
# import package manager
include(${CMAKE_CURRENT_LIST_DIR}/cpm.cmake)
add_package("gh:StableCoder/cmake-scripts#22.01")

# --- set c++ standard ---
include(${cmake-scripts_SOURCE_DIR}/c++-standards.cmake)
cxx_11()
set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

# being a cross-platform target, we enforce standards conformance on MSVC all compile:
# https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang>:-stdlib=libc++>)
add_link_options($<$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang>:-stdlib=libc++>)
add_compile_options($<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->)

if(NOT __DEBUG__ AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # --- link optimizer (?? not work in gcc) ---
  cmake_policy(SET CMP0069 NEW)
  set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)
  include(${cmake-scripts_SOURCE_DIR}/link-time-optimization.cmake)
  link_time_optimization()
endif()

# ##################################################################################################
# sanitizers & static analyzer
# ##################################################################################################
if(NOT ${__DEBUG__})
  my_info("Non-debug mode, skip sanitizers & static analyzer")
  return()
endif()

# cmake-format: off
# warning & error
set(ENABLE_ALL_WARNINGS ON CACHE INTERNAL "")
include(${cmake-scripts_SOURCE_DIR}/compiler-options.cmake)

# format
add_package("gh:TheLartians/Format.cmake@1.7.3")

# enables sanitizers support using the the `USE_SANITIZER` flag available values are: Address,
# Memory, MemoryWithOrigins, Undefined, Thread, Leak, 'Address;Undefined'
# NOTE: need to enable CXX
if (NOT "x${USE_SANITIZER}" STREQUAL "x")
  include(${cmake-scripts_SOURCE_DIR}/sanitizers.cmake)
endif()

if(${USE_STATIC_ANALYZER})
  set(CLANG_TIDY ON CACHE INTERNAL "")
  include(${cmake-scripts_SOURCE_DIR}/tools.cmake)
  clang_tidy(${CLANG_TIDY_ARGS})
endif()
# cmake-format: on
