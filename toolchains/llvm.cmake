set(REAVEROS_LLVM_PARALLEL_LINK_JOBS 8 CACHE STRING "Sets the limit for parallel link jobs of LLVM.")

set(REAVEROS_LLVM_TAG llvmorg-12.0.0-rc3)
ExternalProject_Add(toolchain-llvm
    GIT_REPOSITORY https://github.com/llvm/llvm-project
    GIT_TAG ${REAVEROS_LLVM_TAG}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    DEPENDS toolchain-cmake-install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchains/llvm

    SOURCE_SUBDIR llvm
    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

    LIST_SEPARATOR |

    CMAKE_COMMAND ${REAVEROS_CMAKE}
    CMAKE_ARGS
        -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
        -DCMAKE_C_COMPILER_LAUNCHER=${CMAKE_C_COMPILER_LAUNCHER}
        -DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}
        -DCMAKE_BUILD_TYPE=Release
        -Wno-dev
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DLLVM_TARGETS_TO_BUILD=X86
        -DLLVM_ENABLE_PROJECTS=clang|lld
        -DLLVM_PARALLEL_LINK_JOBS=${REAVEROS_LLVM_PARALLEL_LINK_JOBS}
        -DLLVM_INCLUDE_TESTS=OFF
        -DLLVM_INCLUDE_EXAMPLES=OFF
)
reaveros_add_ep_prune_target(toolchain-llvm)
reaveros_add_ep_fetch_tag_target(toolchain-llvm ${REAVEROS_LLVM_TAG})

set(REAVEROS_LLVM_BINUTILS_EXTRA_TAG binutils-2_36_1)
ExternalProject_Add(toolchain-llvm-binutils-extra
    GIT_REPOSITORY git://sourceware.org/git/binutils-gdb.git
    GIT_TAG ${REAVEROS_LLVM_BINUTILS_EXTRA_TAG}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchains/llvm

    ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

    CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --disable-docs --disable-nls --disable-werror
        --disable-gdb
        "CC=${CMAKE_C_COMPILER_LAUNCHER} ${CMAKE_C_COMPILER}"
        "CXX=${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}"
    BUILD_COMMAND $(MAKE) MAKEINFO=true
    INSTALL_COMMAND $(MAKE) MAKEINFO=true install
)
reaveros_add_ep_prune_target(toolchain-llvm-binutils-extra)
reaveros_add_ep_fetch_tag_target(toolchain-llvm-binutils-extra ${REAVEROS_LLVM_BINUTILS_EXTRA_TAG})

add_dependencies(toolchain-llvm-install toolchain-llvm-binutils-extra-install)

reaveros_register_target(toolchain-llvm-install toolchains)
