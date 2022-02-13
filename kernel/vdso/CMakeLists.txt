file(GLOB thorn_files
    CONFIGURE_DEPENDS
    *.thorn
)

set(_thorn_user_files)
set(_thorn_vdso_files)
set(_thorn_kernel_files)

foreach (abs_thorn_file IN LISTS thorn_files)
    file(RELATIVE_PATH thorn_file "${CMAKE_CURRENT_SOURCE_DIR}" "${abs_thorn_file}")
    string(REGEX REPLACE "\\.[^.]*$" "" base "${thorn_file}")

    list(APPEND _thorn_user_files "${CMAKE_CURRENT_BINARY_DIR}/thorn/user/${base}.h")
    list(APPEND _thorn_vdso_files "${CMAKE_CURRENT_BINARY_DIR}/thorn/vdso/${base}.cpp")
    list(APPEND _thorn_kernel_files "${CMAKE_CURRENT_BINARY_DIR}/thorn/kernel/${base}.cpp")
endforeach()

add_custom_command(
    OUTPUT
        ${_thorn_user_files}
        ${_thorn_vdso_files}
        ${_thorn_kernel_files}

    COMMENT "Compiling syscall definitions from thorn files..."
    COMMAND "${REAVEROS_THORN}" -i "${CMAKE_CURRENT_SOURCE_DIR}" -o "${CMAKE_CURRENT_BINARY_DIR}/thorn" -s "rose::syscall" -a ${REAVEROS_ARCH}
    DEPENDS ${REAVEROS_THORN} ${thorn_files}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set_source_files_properties(
    FILES
        ${_thorn_user_files}
        ${_thorn_vdso_files}
        ${_thorn_kernel_files}
    PROPERTIES
        GENERATED TRUE
)

add_library(vdso SHARED
    ${_thorn_vdso_files}
)

install(FILES ${_thorn_user_files}
    DESTINATION ${CMAKE_SYSROOT}/include/rose/syscall
)

add_library(syscall_table STATIC
    ${_thorn_kernel_files}
)

set_target_properties(syscall_table
    PROPERTIES
        COMPILE_FLAGS "-I${CMAKE_SOURCE_DIR}"
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

set_source_files_properties(
    ${CMAKE_CURRENT_BINARY_DIR}/incbin_vdso.asm
    PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libvdso.so
)

add_dependencies(vdso-incbin vdso)

set(vdso_symbols_file "${CMAKE_CURRENT_BINARY_DIR}/vdso_symbols.lds")
set(vdso_symbols_file "${vdso_symbols_file}" PARENT_SCOPE)

add_custom_command(OUTPUT ${vdso_symbols_file}
    COMMAND ${CMAKE_READELF} --dyn-syms $<TARGET_FILE:vdso> | tail -n+4 | grep 'FUNC' | grep -v __rose_syscall | awk \'{ printf \"%s = __vdso_base + 0x%s\;\\n\", $$8, $$2; }\' > ${vdso_symbols_file}
    DEPENDS vdso
)

add_custom_target(vdso_symbols DEPENDS ${vdso_symbols_file})

set(vdso_offsets_file "${CMAKE_CURRENT_BINARY_DIR}/vdso_offsets.lds")
set(vdso_offsets_file "${vdso_offsets_file}" PARENT_SCOPE)

add_custom_command(OUTPUT ${vdso_offsets_file}
    COMMAND ${CMAKE_READELF} --dyn-syms $<TARGET_FILE:vdso> | tail -n+4 | grep __rose_syscall | awk \'{ printf \"%s = __vdso_base + 0x%s\;\\n\", $$8, $$2; }\' > ${vdso_offsets_file}
    DEPENDS vdso
)

add_custom_target(vdso_offsets DEPENDS ${vdso_offsets_file})