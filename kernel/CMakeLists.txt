cmake_minimum_required(VERSION 3.20)
project(reaveros-kernel ASM CXX)

if (NOT REAVEROS_ENABLE_UNIT_TESTS)
    add_subdirectory(vdso)
    add_subdirectory(bootinit)

    file(GLOB_RECURSE sources
        CONFIGURE_DEPENDS
        boot/*.cpp
        memory/*.cpp
        time/*.cpp
        scheduler/*.cpp
        util/*.cpp
    )

    file(GLOB_RECURSE current_arch_sources
        CONFIGURE_DEPENDS
        arch/common/*.cpp
        arch/${REAVEROS_ARCH}/*.cpp
        arch/${REAVEROS_ARCH}/*.asm
    )

    add_executable(kernel-elf
        main.cpp
        ${sources}
        ${current_arch_sources}
    )

    target_link_libraries(kernel-elf
        bootinit-incbin
        vdso-incbin
        ${CMAKE_SYSROOT}/usr/lib/librosestd.a
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
            ${CMAKE_OBJCOPY} -O binary -j .text -j .data
                $<TARGET_FILE:kernel-elf>
                ${CMAKE_BINARY_DIR}/kernel
    )

    add_custom_target(kernel-flat ALL DEPENDS ${CMAKE_BINARY_DIR}/kernel)

    install(FILES ${CMAKE_BINARY_DIR}/kernel
        DESTINATION ${CMAKE_INSTALL_PREFIX}
    )
else()
    enable_testing()

    add_subdirectory(test)
endif()
