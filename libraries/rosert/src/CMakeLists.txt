set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

file(GLOB crt_sources
    ${REAVEROS_ARCH}/crt*.asm
)

add_library(rosert-crt STATIC
    ${crt_sources}
)

install(TARGETS rosert-crt
    DESTINATION ${install_dir}
)

file(GLOB sources
    ${REAVEROS_ARCH}/*.asm
    *.cpp
    ${REAVEROS_ARCH}/*.cpp
)

list(REMOVE_ITEM sources ${crt_sources})

# add_library(rosert STATIC
# ${sources}
# )

# install(TARGETS rosert
# DESTINATION ${install_dir}
# )
