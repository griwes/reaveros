set(REAVEROS_DOSFSTOOLS_TAG v4.2)
ExternalProject_Add(toolchain-dosfstools
    GIT_REPOSITORY https://github.com/dosfstools/dosfstools
    GIT_TAG ${REAVEROS_DOSFSTOOLS_TAG}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchains/dosfstools

    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

    CONFIGURE_COMMAND cd <SOURCE_DIR> && <SOURCE_DIR>/autogen.sh
    COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        "CC=${CMAKE_C_COMPILER_LAUNCHER} ${CMAKE_C_COMPILER}"
        "CXX=${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}"
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install
)
reaveros_add_ep_prune_target(toolchain-dosfstools)
reaveros_add_ep_fetch_tag_target(toolchain-dosfstools ${REAVEROS_DOSFSTOOLS_TAG})

reaveros_register_target(toolchain-dosfstools-install toolchains)

ExternalProject_Add(toolchain-mtools
    URL ftp://ftp.gnu.org/gnu/mtools/mtools-4.0.26.tar.gz
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchains/mtools

    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUIL}

    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        "CC=${CMAKE_C_COMPILER_LAUNCHER} ${CMAKE_C_COMPILER}"
        "CXX=${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}"
    BUILD_COMMAND $(MAKE)
    INSTALL_COMMAND $(MAKE) install
)
reaveros_add_ep_prune_target(toolchain-mtools)

reaveros_register_target(toolchain-mtools-install toolchains)
