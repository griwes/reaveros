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

add_executable(kernel-elf
    ${sources}
)

target_link_libraries(kernel-elf
    ${CMAKE_SYSROOT}/usr/lib/libfreestanding.a
)

set(linker_script ${CMAKE_CURRENT_SOURCE_DIR}/arch/${REAVEROS_ARCH}/kernel.lds)

set_target_properties(kernel-elf
    PROPERTIES
        COMPILE_FLAGS "-g -mcmodel=kernel -DREAVEROS_KERNEL_SOURCE_ROOT='\"${CMAKE_SOURCE_DIR}\"'"
        LINK_FLAGS "-T ${linker_script} -static -Map=kernel.map -nostdlib"
        LINK_DEPENDS ${linker_script}
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/kernel
    DEPENDS kernel-elf

    COMMAND
        ${CMAKE_OBJCOPY} -O binary -j .text -j .rodata -j .data -j .bss --set-section-flags .bss=alloc,load,contents
            $<TARGET_FILE:kernel-elf>
            ${CMAKE_BINARY_DIR}/kernel
)

add_custom_target(kernel-flat ALL DEPENDS ${CMAKE_BINARY_DIR}/kernel)

install(FILES ${CMAKE_BINARY_DIR}/kernel
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
