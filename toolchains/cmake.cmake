ExternalProject_Add(toolchain-cmake
    GIT_REPOSITORY https://github.com/Kitware/CMake
    GIT_TAG v3.20.0-rc3
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchains/cmake

    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

    PATCH_COMMAND git reset --hard
    #COMMAND git apply < ${CMAKE_CURRENT_SOURCE_DIR}/cmake-000-propagate-jobserver-in-make-unconditionally.patch

    CMAKE_ARGS
        -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

#add_custom_command(TARGET toolchain-cmake-install



