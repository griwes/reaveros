reaveros_add_aggregate_targets(loaders)

if ("amd64" IN_LIST REAVEROS_ARCHITECTURES AND "uefi" IN_LIST REAVEROS_LOADERS)
    reaveros_add_aggregate_targets(loaders-uefi)

    foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
        ExternalProject_Add(loader-uefi-amd64-${toolchain}
            DOWNLOAD_COMMAND ""
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/uefi
            BUILD_ALWAYS 1

            STEP_TARGETS install

            DEPENDS toolchain-llvm-install all-${toolchain}-amd64-kernel-libraries

            INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/loaders/uefi-amd64-${toolchain}

            ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

            CMAKE_COMMAND ${REAVEROS_CMAKE}
            CMAKE_ARGS
                -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
                -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
                -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_TOOLCHAIN_FILE=${REAVEROS_BINARY_DIR}/install/toolchains/${toolchain}-amd64-kernel.cmake
                -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
                -DREAVEROS_ARCH=amd64
        )

        reaveros_register_target(loader-uefi-amd64-${toolchain} ${toolchain} amd64 loaders uefi)
    endforeach()
endif()

