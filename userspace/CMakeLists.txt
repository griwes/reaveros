reaveros_add_aggregate_targets(userspace)

add_subdirectory(services)

foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
    set(_bootinit_name userspace-bootinit-${architecture})

    ExternalProject_Add(${_bootinit_name}
        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bootinit
        BUILD_ALWAYS 1

        STEP_TARGETS install

        DEPENDS toolchain-llvm-install all-${architecture}-freestanding-libraries

        INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/userspace/bootinit/${architecture}

        ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

        CMAKE_COMMAND ${REAVEROS_CMAKE}
        CMAKE_ARGS
            -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
            -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_TOOLCHAIN_FILE=${REAVEROS_BINARY_DIR}/install/toolchain/files/${architecture}-freestanding.cmake
            -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    )

    reaveros_register_target(${_bootinit_name} ${architecture} userspace)
endforeach()

