set(_binary_dir ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_CURRENT_BINARY_DIR ${_binary_dir}/kernel)

reaveros_add_aggregate_targets(kernels)

function(_reaveros_add_kernel_config architecture toolchain)
    set(target_name kernel-${architecture}-${toolchain})

    ExternalProject_Add(${target_name}
        EXCLUDE_FROM_ALL TRUE

        DOWNLOAD_COMMAND ""
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/kernel
        BUILD_ALWAYS 1

        STEP_TARGETS install

        DEPENDS toolchain-${toolchain}-install all-${toolchain}-${architecture}-kernel-libraries

        INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/kernels/${architecture}-${toolchain}

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

    reaveros_register_target(${target_name} ${toolchain} ${architecture} kernels)
endfunction()

foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
    foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
        if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${directory})
            _reaveros_add_kernel_config(
                ${architecture}
                ${toolchain}
            )
        endif()
    endforeach()
endforeach()

set(CMAKE_CURRENT_BINARY_DIR ${_binary_dir})
