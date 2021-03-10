file(GLOB toolchain_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/files ${CMAKE_CURRENT_SOURCE_DIR}/files/*.cmake.in)

foreach (toolchain_file_in IN LISTS toolchain_files)
    string(REGEX
        REPLACE ".in$" ""
        toolchain_file
        ${toolchain_file_in}
    )

    configure_file(
        ${REAVEROS_SOURCE_DIR}/toolchains/files/${toolchain_file_in}
        ${REAVEROS_BINARY_DIR}/install/toolchains/${toolchain_file}
        @ONLY
    )
endforeach()

include(cmake.cmake)

if ("uefi" IN_LIST REAVEROS_LOADERS)
    include(fs-fat.cmake)
endif()

if ("llvm" IN_LIST REAVEROS_TOOLCHAINS)
    include(llvm.cmake)
endif()
