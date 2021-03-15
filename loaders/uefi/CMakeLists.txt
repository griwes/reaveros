cmake_minimum_required(VERSION 3.20)
project(reaveros-loader-uefi ASM CXX)

file(GLOB_RECURSE sources
    CONFIGURE_DEPENDS
    *.cpp
)

set_source_files_properties(
    crt0.asm
    PROPERTIES
        LANGUAGE ASM
)

add_library(crt0 STATIC
    crt0.asm
)

add_executable(loader-uefi-elf
    ${sources}
)

target_link_libraries(loader-uefi-elf
    crt0
    freestanding
)

set(linker_script ${CMAKE_CURRENT_SOURCE_DIR}/loader.lds)

set_target_properties(loader-uefi-elf
    PROPERTIES
        COMPILE_FLAGS "-DHAVE_USE_MS_ABI -fshort-wchar"
        LINK_FLAGS "-T ${linker_script} -Bsymbolic"
        LINK_DEPENDS ${linker_script}
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/loader-uefi
    DEPENDS loader-uefi-elf

    COMMAND
        ${CMAKE_OBJCOPY} -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64
            $<TARGET_FILE:loader-uefi-elf>
            ${CMAKE_BINARY_DIR}/loader-uefi
)

add_custom_target(loader-uefi-pe ALL DEPENDS ${CMAKE_BINARY_DIR}/loader-uefi)

install(FILES ${CMAKE_BINARY_DIR}/loader-uefi
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
