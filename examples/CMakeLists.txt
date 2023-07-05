cmake_minimum_required(VERSION 3.14...3.22)

project(ccl_examples LANGUAGES CXX)

# --- Import tools ----
include(../cmake/common.cmake)
cxx_20()

find_package(Boost 1.72 REQUIRED)
find_package(Threads REQUIRED)

add_local_package(
  NAME ccl2
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..
  OPTIONS "CCL2_WITH_YYJSON ON" "CCL2_WITH_FMT ON" "CCL2_WITH_COROUTINES ON" "CCL2_WITH_HTTP ON"
)

macro(add_example_impl TARGET RELATIVE_DIR)
  set(multiValueArgs SRCS DEPENDS)
  cmake_parse_arguments(EX "" "" "${multiValueArgs}" ${ARGN})

  list(LENGTH EX_SRCS SRCS_CNT)
  if(${SRCS_CNT})
    add_executable(${TARGET} ${EX_SRCS})
  else()
    add_executable(${TARGET} ${RELATIVE_DIR}/${TARGET}.cpp)
  endif()
  target_link_libraries(${TARGET} PRIVATE ccl2::ccl2 ${EX_DEPENDS})
endmacro(add_example_impl)

# ---- Create standalone executable ----
macro(add_example TARGET)
  add_example_impl(${TARGET} src ${ARGN})
endmacro(add_example)

add_example(json)
add_example(function)
add_example(scope_exit)
add_example(signal DEPENDS Threads::Threads)
add_example(service)
add_example(stopwatch)

# add_example(udp_sender DEPENDS Threads::Threads) add_example(udp_recver DEPENDS Threads::Threads)
add_example(asio_pool)
# add_example(asio_pool_udp_server DEPENDS Threads::Threads std::coroutines)

# add_example(url DEPENDS Boost::headers)
macro(add_coro_example TARGET)
  add_example_impl(${TARGET} src/coro ${ARGN})
endmacro(add_coro_example)

add_coro_example(asio_pool_with_coroutine)
add_coro_example(udp_sender)
add_coro_example(udp_recver)
add_coro_example(http_server)
add_coro_example(callback_wrapper)
add_coro_example(async_queue)
add_coro_example(async_signal)
add_coro_example(async_signal2)
