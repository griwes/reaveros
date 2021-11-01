set(REAVEROS_LLVM_PARALLEL_LINK_JOBS 8 CACHE STRING "Sets the limit for parallel link jobs of LLVM.")

ExternalProject_Add(toolchain-llvm
    GIT_REPOSITORY ${REAVEROS_LLVM_REPO}
    GIT_TAG ${REAVEROS_LLVM_TAG}
    GIT_SHALLOW TRUE
    UPDATE_DISCONNECTED 1

    STEP_TARGETS configure install

    DEPENDS toolchain-cmake-install

    INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchain/llvm

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
        -DLLVM_ENABLE_RUNTIMES=compiler-rt|libcxx|libcxxabi
        -DCOMPILER_RT_BUILD_BUILTINS=ON
        -DCOMPILER_RT_BUILD_LIBFUZZER=OFF
        -DCOMPILER_RT_BUILD_MEMPROF=OFF
        -DCOMPILER_RT_BUILD_PROFILE=OFF
        -DCOMPILER_RT_BUILD_SANITIZERS=OFF
        -DCOMPILER_RT_BUILD_XRAY=OFF
        -DLLVM_PARALLEL_LINK_JOBS=${REAVEROS_LLVM_PARALLEL_LINK_JOBS}
        -DLLVM_INCLUDE_TESTS=OFF
        -DLLVM_INCLUDE_EXAMPLES=OFF
)
reaveros_add_ep_prune_target(toolchain-llvm)
reaveros_add_ep_fetch_tag_target(toolchain-llvm)

if ("amd64" IN_LIST REAVEROS_ARCHITECTURES AND "uefi" IN_LIST REAVEROS_LOADERS)
    ExternalProject_Add(toolchain-llvm-binutils-extra
        GIT_REPOSITORY ${REAVEROS_LLVM_BINUTILS_EXTRA_REPO}
        GIT_TAG ${REAVEROS_LLVM_BINUTILS_EXTRA_TAG}
        GIT_SHALLOW TRUE
        UPDATE_DISCONNECTED 1

        STEP_TARGETS install

        INSTALL_DIR ${REAVEROS_BINARY_DIR}/install/toolchain/llvm

        ${_REAVEROS_CONFIGURE_HANDLED_BY_BUILD}

        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR> --disable-docs --disable-nls --disable-werror
            --disable-gdb
            --target=x86_64-w64-mingw32
            "CC=${CMAKE_C_COMPILER_LAUNCHER} ${CMAKE_C_COMPILER}"
            "CXX=${CMAKE_CXX_COMPILER_LAUNCHER} ${CMAKE_CXX_COMPILER}"
        BUILD_COMMAND $(MAKE) MAKEINFO=true
        INSTALL_COMMAND $(MAKE) MAKEINFO=true install
    )
    reaveros_add_ep_prune_target(toolchain-llvm-binutils-extra)
    reaveros_add_ep_fetch_tag_target(toolchain-llvm-binutils-extra)

    add_dependencies(toolchain-llvm-install toolchain-llvm-binutils-extra-install)
endif()

# install compiler-rt to the appropriate sysroots
string(REGEX REPLACE "llvmorg-([0-9]+\.[0-9]+\.[0-9])+(-.*)?" "\\1" _llvm_version "${REAVEROS_LLVM_TAG}")

set(_reaveros_clang_path_amd64 "x86_64-unknown-linux-gnu")

foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
    foreach (mode IN ITEMS freestanding hosted)
        set(_sub_path "${_llvm_version}/lib/${_reaveros_clang_path_${architecture}}")
        set(_builtin_lib "${REAVEROS_BINARY_DIR}/install/toolchain/llvm/lib/clang/${_sub_path}/libclang_rt.builtins.a")
        set(_destination "${REAVEROS_BINARY_DIR}/install/sysroots/${architecture}-${mode}/usr/lib")

        add_custom_command(TARGET toolchain-llvm
            COMMAND mkdir -p ${_destination}
            COMMAND cp ${_builtin_lib} ${_destination}
        )
        add_custom_command(TARGET toolchain-llvm-install
            COMMAND mkdir -p ${_destination}
            COMMAND cp ${_builtin_lib} ${_destination}
        )
    endforeach()
endforeach()

reaveros_register_target(toolchain-llvm-install toolchain)
