reaveros_add_aggregate_targets(libraries)

function(_reaveros_libraries_add_library directory mode architecture toolchain)
    set(library_name library-${directory}-${mode}-${architecture}-${toolchain})

    ExternalProject_Add(${library_name}
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${directory}
        BUILD_ALWAYS 1

        STEP_TARGETS install

        DEPENDS toolchain-${toolchain}-install

        INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/sysroots/${toolchain}-${architecture}-${mode}

        ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

        CMAKE_COMMAND ${REAVEROS_CMAKE}
        CMAKE_ARGS
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
            -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_TOOLCHAIN_FILE=${REAVEROS_BINARY_DIR}/install/toolchains/${toolchain}-${architecture}-${mode}.cmake
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    reaveros_register_target(${library_name}-install ${toolchain} ${architecture} ${mode} libraries)
endfunction()

file(GLOB directories RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
    foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
        foreach (directory IN LISTS directories)
            foreach (mode IN ITEMS kernel user)
                if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${directory})
                    _reaveros_libraries_add_library(
                        ${directory}
                        ${mode}
                        ${architecture}
                        ${toolchain}
                    )
                endif()
            endforeach()
        endforeach()
    endforeach()
endforeach()

