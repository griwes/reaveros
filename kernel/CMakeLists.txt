cmake_minimum_required(VERSION 3.20)
project(reaveros-kernel ASM CXX)

file(GLOB_RECURSE sources
    CONFIGURE_DEPENDS
    *.cpp
)

file(GLOB_RECURSE arch_sources
    CONFIGURE_DEPENDS
    arch/*.cpp
)

file(GLOB_RECURSE current_arch_sources
    CONFIGURE_DEPENDS
    arch/${REAVEROS_ARCH}/*.cpp
)

# Without current arch sources, arch_sources will be a list of sources of *other* arches.
list(REMOVE_ITEM arch_sources ${current_arch_sources})
# And we can just remove that from the main list of sources.
list(REMOVE_ITEM sources ${arch_sources})

add_executable(kernel
    ${sources}
)

target_link_libraries(kernel
    freestanding
)

set(linker_script ${CMAKE_CURRENT_SOURCE_DIR}/arch/${REAVEROS_ARCH}/kernel.lds)

set_target_properties(kernel
    PROPERTIES
        COMPILE_FLAGS "-mcmodel=kernel"
        LINK_FLAGS "-T ${linker_script} --oformat binary -static -Map=kernel.map -nostdlib"
        LINK_DEPENDS ${linker_script}
)

install(TARGETS kernel
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
