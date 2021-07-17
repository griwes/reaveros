cmake_minimum_required(VERSION 3.12)
project(reaveros-meta C CXX)

set(REAVEROS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(REAVEROS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(REAVEROS_C_STANDARD "11")
set(REAVEROS_CXX_STANDARD "20")

option(REAVEROS_USE_CACHE "Enable the use of a cache program in builds." ON)
set(REAVEROS_CACHE_PROGRAM "ccache" CACHE STRING "Name of the cache program to use (defaults to ccache).")

set(REAVEROS_CMAKE ${REAVEROS_BINARY_DIR}/install/toolchain/cmake/bin/cmake)

if (REAVEROS_USE_CACHE)
    find_program(_cache_program ${REAVEROS_CACHE_PROGRAM})
    if (_cache_program)
        set(CMAKE_C_COMPILER_LAUNCHER ${_cache_program})
        set(CMAKE_CXX_COMPILER_LAUNCHER ${_cache_program})
    endif()
endif()

if (NOT GIT_EXECUTABLE OR NOT GIT_VERSION_STRING)
    find_package(Git QUIET)
    if (NOT GIT_EXECUTABLE)
        message(FATAL_ERROR "error: could not find git")
    endif()
endif()

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.20)
    cmake_policy(SET CMP0114 NEW)
endif()
include(ExternalProject)

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.20)
    set(_REAVEROS_CONFIGURE_HANDLED_BY_BUILD CONFIGURE_HANDLED_BY_BUILD TRUE)
else()
    set(_REAVEROS_CONFIGURE_HANDLED_BY_BUILD "")
endif()

set(REAVEROS_ARCHITECTURES "amd64" CACHE STRING "List of architectures to build for. Possible values: amd64.")
set(REAVEROS_LOADERS "uefi" CACHE STRING "List of loaders to build images with. Possible values: uefi (amd64 only).")

include(cmake/target_functions.cmake)

reaveros_add_aggregate_targets("")
add_custom_target(all-toolchain)

add_subdirectory(toolchain EXCLUDE_FROM_ALL)
add_subdirectory(libraries EXCLUDE_FROM_ALL)
add_subdirectory(loaders EXCLUDE_FROM_ALL)
include(kernel/meta.cmake EXCLUDE_FROM_ALL)
add_subdirectory(userspace EXCLUDE_FROM_ALL)
add_subdirectory(images EXCLUDE_FROM_ALL)
