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


include("${CMAKE_SOURCE_DIR}/vendor/rapidjson.cmake")

set(CPP_CORO_LIB ${CMAKE_SOURCE_DIR}/vendor/cppcoro/build/darwin_x64_clang10.0.0_debug/lib/libcppcoro.a)
set(CPP_CORO_INCLUDE ${CMAKE_SOURCE_DIR}/vendor/cppcoro/include)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

ADD_LIBRARY(cppcoro STATIC IMPORTED)
SET_TARGET_PROPERTIES(cppcoro PROPERTIES IMPORTED_LOCATION ${CPP_CORO_LIB})

# ---------------------------------------------------------------------------
# Includes
# ---------------------------------------------------------------------------

include_directories(
        ${CMAKE_SOURCE_DIR}/include
        ${CPP_CORO_INCLUDE}
        ${RAPID_JSON_INCLUDE}
)

if(Boost_FOUND)
    include_directories( ${Boost_INCLUDE_DIRS} )
endif()

set(INCLUDE_H, include/async_task.hpp)

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------

set(SRC_HELLO_CC src/hello_coro.cpp)
set(SRC_SERVER_CC src/asyc_server.cpp)
# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------

add_executable(hello_coro ${SRC_HELLO_CC} ${INCLUDE_H})
target_compile_options(hello_coro PRIVATE -fcoroutines-ts)
target_link_libraries(hello_coro Threads::Threads cppcoro)

add_executable(async_server ${SRC_SERVER_CC} ${INCLUDE_H})
target_compile_options(async_server PRIVATE -fcoroutines-ts)
target_link_libraries(async_server Threads::Threads cppcoro Boost::boost Boost::thread rapidjson)