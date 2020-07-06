cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Environment
# ---------------------------------------------------------------------------
project(async_demo)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -std=c++17)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

set(CPP_CORO_LIB ${CMAKE_SOURCE_DIR}/vendor/cppcoro/build/darwin_x64_clang10.0.0_debug/lib/libcppcoro.a)
set(CPP_CORO_INCLUDE ${CMAKE_SOURCE_DIR}/vendor/cppcoro/include)
find_package(Threads REQUIRED)

ADD_LIBRARY(cppcoro STATIC IMPORTED)
SET_TARGET_PROPERTIES(cppcoro PROPERTIES IMPORTED_LOCATION ${CPP_CORO_LIB})

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------

include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CPP_CORO_INCLUDE}
)

set(INCLUDE_H)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

set(SRC_HELLO_CC src/hello_coro.cpp include/async_task.hpp)

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

add_executable(hello_coro ${SRC_HELLO_CC} ${INCLUDE_H})
target_compile_options(hello_coro PRIVATE -fcoroutines-ts)
target_link_libraries(hello_coro Threads::Threads cppcoro)