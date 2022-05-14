cmake_minimum_required(VERSION 3.18)

set(CMAKE_CXX_COMPILER_WORKS 1)

# cut a circular dependency caused by our compiler inserting -lrosestd -lrose-crt
set(CMAKE_CXX_COMPILER_WORKS 1)
project(reaveros-rosert ASM CXX)

if (NOT REAVEROS_IS_FREESTANDING AND NOT REAVEROS_ENABLE_UNIT_TESTS)
    add_subdirectory(include)
    add_subdirectory(src)
else()
    install(TARGETS)
endif()

