ExternalProject_Add(toolchain-cmake
    GIT_REPOSITORY ${REAVEROS_CMAKE_REPO}
    GIT_TAG ${REAVEROS_CMAKE_TAG}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchain/cmake

    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

    CMAKE_ARGS
        -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)
reaveros_add_ep_prune_target(toolchain-cmake)
reaveros_add_ep_fetch_tag_target(toolchain-cmake)

reaveros_register_target(toolchain-cmake-install toolchain)
