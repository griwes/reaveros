add_library(vdso SHARED
    src/early.cpp
)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/incbin_vdso.asm
    "
    .section .vdso
    .globl begin_vdso
    .globl end_vdso
    begin_vdso:
    .incbin \"${CMAKE_CURRENT_BINARY_DIR}/libvdso.so\"
    end_vdso:
    "
)

add_library(vdso-incbin STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/incbin_vdso.asm
)

add_dependencies(vdso-incbin vdso)

set(vdso_symbols_file "${CMAKE_CURRENT_BINARY_DIR}/vdso_symbols.lds")
set(vdso_symbols_file "${vdso_symbols_file}" PARENT_SCOPE)

add_custom_command(OUTPUT ${vdso_symbols_file}
    COMMAND ${CMAKE_READELF} --dyn-syms $<TARGET_FILE:vdso> | tail -n-3 | grep 'FUNC' | grep -v __rose_syscall | awk \'{ printf \"%s = __vdso_base + 0x%s\;\\n\", $$8, $$2; }\' > ${vdso_symbols_file}
    DEPENDS vdso
)

add_custom_target(vdso_symbols DEPENDS ${vdso_symbols_file})
