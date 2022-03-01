add_executable(bootinit-elf
    main.cpp
    log.cpp
)

target_link_libraries(bootinit-elf
    ${CMAKE_SYSROOT}/usr/lib/librosestd.a
)

set(linker_script_in ${CMAKE_CURRENT_SOURCE_DIR}/bootinit.lds.in)
set(linker_script ${CMAKE_CURRENT_BINARY_DIR}/bootinit.lds)

configure_file(${linker_script_in} ${linker_script} @ONLY)

set_target_properties(bootinit-elf
    PROPERTIES
        COMPILE_FLAGS "-g"
        LINK_FLAGS "-T ${linker_script} -static -Map=bootinit.map -nostdlib"
        LINK_DEPENDS ${linker_script}
)

add_dependencies(bootinit-elf
    vdso_symbols
)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bootinit
    DEPENDS bootinit-elf

    COMMAND
        ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:bootinit-elf> ${CMAKE_CURRENT_BINARY_DIR}/bootinit
            --set-section-flags .data=alloc,load,contents
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/incbin_bootinit.asm
    "
    .section .bootinit
    .globl begin_bootinit
    .globl end_bootinit
    begin_bootinit:
    .incbin \"${CMAKE_CURRENT_BINARY_DIR}/bootinit\"
    end_bootinit:
    "
)

add_library(bootinit-incbin STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/incbin_bootinit.asm
)

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/incbin_bootinit.asm
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/bootinit
)
