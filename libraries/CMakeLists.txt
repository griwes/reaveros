reaveros_add_aggregate_targets(libraries)

function(_reaveros_libraries_add_library directory mode architecture)
    set(library_name library-${directory}-${mode}-${architecture})

    ExternalProject_Add(${library_name}
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${directory}
        BUILD_ALWAYS 1

        STEP_TARGETS install

        DEPENDS toolchain-llvm-install

        INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/sysroots/${architecture}-${mode}

        ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

        CMAKE_COMMAND ${REAVEROS_CMAKE}
        CMAKE_ARGS
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
            -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_TOOLCHAIN_FILE=${REAVEROS_BINARY_DIR}/install/toolchain/files/${architecture}-${mode}.cmake
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    reaveros_register_target(${library_name}-install ${architecture} ${mode} libraries)
endfunction()

set(freestanding_libraries
    libfreestanding
    libboot-protocol
)

set(hosted_libraries
    libfreestanding
)

foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
    foreach (mode IN ITEMS freestanding hosted)
        foreach (directory IN LISTS ${mode}_libraries)
            if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${directory})
                _reaveros_libraries_add_library(
                    ${directory}
                    ${mode}
                    ${architecture}
                )
            endif()
        endforeach()
    endforeach()
endforeach()

