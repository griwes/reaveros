# ReaverOS

A new iteration, from "scratch", of a µkernel-based operating system for 64-bit architectures. Previous iteration can be found
here: https://github.com/griwes/reaveros-iteration1.

## Building ReaverOS

### Dependencies

ReaverOS' build system bootstraps most tools that it uses as build tools, but not all the tools the tools it bootstraps depend
on themselves. That being said, we are trying to keep the dependencies of the build system to a minimum. Currently, those
dependencies are:

* CMake (3.12 or higher);
* Python (3.7 or higher);
* Git;
* Bison;
* Flex;
* OpenSSL;
* C and C++ compilers;
* Make.

On a recent apt-based systems, the following command should fulfill the dependencies:

```
apt install build-essential cmake git python3 bison flex libssl-dev
```

### Build considerations

ReaverOS builds the full toolchain that it uses for all builds internally, at versions fixed as git tags. This means that to
build ReaverOS, you need internet access to fetch the repositories for all of the toolchains.

This also means that an initial build using a given toolchain will take a long time, because it will build either full LLVM
or full GCC+binutils with their dependencies. A docker image containing an already built toolchain will be provided at a future
date, once the infrastructure for it has been prepared, to allow for building ReaverOS in a fresh checkout without requiring
redoing a full toolchain build every time.

To avoid rebuilding the toolchains from scratch when you do `make clean`, it is recommended to have `ccache` installed and
enabled (see options below).

### Configuration options

The build uses the normal CMake C and C++ compiler heuristics to detect what compiler to use for building all the various tools
used during the build. Additionally, the following options are exposed:

* `REAVEROS_USE_CACHE` - enable using a caching application for builds (this could be `ccache`, `sccache`, but also other
compiler launchers like `icecc`);
* `REAVEROS_CACHE_PROGRAM` - specify the binary for the caching application (defaults to `ccache`);
* `REAVEROS_TOOLCHAINS` - select the toolchains to be enabled for the build; currently only `llvm` is supported;
* `REAVEROS_ARCHITECTURES` - select the target CPU architectures to be enabled; currently only `amd64` is supported;
* `REAVEROS_LOADERS` - select the bootloaders to be enabled; currently only `uefi` is supported.

### Build targets

ReaverOS' build system doesn't add any targets as a dependency of `all`, due to the sheer amount of targets an eventual full
configuration will require. Instead, it provides more fine grained aggregate targets. There is too many of those to list them
here; you can run `make help` to see them all. They should be self explanatory, but to name a few as an example:

* `all-llvm-loaders-uefi` - will build UEFI bootloaders for all enabled architectures, using the LLVM toolchain;
* `all-amd64-user-libraries` - will build all usermode libraries for amd64 using all enabled toolchains.

Some of the aggregate targets are not useful and are only generated to allow for code simplicity in the functions creating them;
for instance, `all-user-loaders` is not a reasonable target to run, because bootloaders fall neither into the userspace nor into
kernel space (though they do use kernel-mode built libraries in their own builds).

There are also targets for the specific components of the build, for instance:

* `toolchain-llvm`;
* `library-libfreestanding-kernel-amd64-llvm`;
* `image-uefi-efipart-amd64-llvm`.

There's one more group of targets that are of note - the prune targets. All toolchain targets have an associated prune target,
and there also exists an aggregate `all-toolchains-prune` target. Their role is to remove the source and build directories of
a given (or all) toolchain subprojects, to reduce the build directory size; since all the toolchains install into subdirectories
of `install/toolchains`, the source and build directories aren't needed to build all of ReaverOS targets.

Do keep in mind that this means that if a version of a pruned toolchain changes, you will need to re-download the sources,
re-configure the project, and rebuild it fully; since the compilers used by ReaverOS can (and will) take a long time to compile,
you should be very careful of pruning a toolchain if you disabled build caching (see `REAVEROS_USE_CACHE` above), built enough
files to have your cache evict the previous stored build results, and do not wish to possibly wait for full GCC or LLVM builds
to finish whenever you pull the main branch.

### Running the OS (or, for now, its UEFI bootloader)

This last mentioned target - `image-uefi-efipart-amd64-llvm` - will create a file containing a FAT filesystem image that includes
the UEFI bootloader for the amd64 architecture. If you install QEMU and OVMF, you can then run it and see what happens with a
command like this (this is the exact command I have been using to test at the time of these words being written on a Debian
sid):

```
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive format=raw,file=install/images/uefi-efipart-amd64-llvm.img -monitor stdio -parallel file:/dev/stdout -cpu qemu64,+sse3,+sse4.1,+sse4.2 -m 2048 -smp 4 -vga std -M pc-i440fx-2.1 -netdev user,id=net0 -device e1000e,romfile=,netdev=net0
```

