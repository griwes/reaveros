set_directory_properties(
    PROPERTIES
        CLEAN_NO_CUSTOM 1
)

file(GLOB toolchain_files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/files ${CMAKE_CURRENT_SOURCE_DIR}/files/*.cmake.in)

foreach (toolchain_file_in IN LISTS toolchain_files)
    string(REGEX
        REPLACE ".in$" ""
        toolchain_file
        ${toolchain_file_in}
    )

    configure_file(
        ${REAVEROS_SOURCE_DIR}/toolchain/files/${toolchain_file_in}
        ${REAVEROS_BINARY_DIR}/install/toolchain/files/${toolchain_file}
        @ONLY
    )
endforeach()

add_custom_target(all-toolchain-prune)

include(config.cmake)

include(cmake.cmake)
include(llvm.cmake)

if ("uefi" IN_LIST REAVEROS_LOADERS)
    include(fs-fat.cmake)
endif()

include(thorn.cmake)
