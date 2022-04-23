set(patch_files
    ${CMAKE_CURRENT_LIST_DIR}/cmake/patches/000-reaveros.patch
)

add_custom_command(OUTPUT cmake-patch-timestamp
    COMMAND touch cmake-patch-timestamp
    DEPENDS ${patch_files}
)

add_custom_target(cmake-patch-timestamp-target
    DEPENDS cmake-patch-timestamp
)

ExternalProject_Add(toolchain-cmake
    GIT_REPOSITORY ${REAVEROS_CMAKE_REPO}
    GIT_TAG ${REAVEROS_CMAKE_TAG}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    DEPENDS cmake-patch-timestamp-target

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchain/cmake

    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

    PATCH_COMMAND git reset --hard && git clean -fxd && git apply ${patch_files}

    CMAKE_ARGS
        -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)
reaveros_add_ep_prune_target(toolchain-cmake)
reaveros_add_ep_fetch_tag_target(toolchain-cmake)

reaveros_register_target(toolchain-cmake-install toolchain)
