# this script adds all subprojects to a single build to allow IDEs understand the full project
# structure.
cmake_minimum_required(VERSION 3.14...3.22)

project(BuildAll LANGUAGES CXX)

include(../cmake/common.cmake)

add_package("gh:TheLartians/Format.cmake@1.7.3")

# examples
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../examples ${CMAKE_BINARY_DIR}/examples)

# add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../bench ${CMAKE_BINARY_DIR}/bench)

# test
if(NOT CMAKE_CROSSCOMPILING)
  # needed to generate test target
  enable_testing()
  add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../test ${CMAKE_BINARY_DIR}/test)
endif()
