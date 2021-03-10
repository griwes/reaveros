file(GLOB sources *.cpp)

add_library(freestanding STATIC
    ${sources}
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

install(TARGETS freestanding
    DESTINATION ${install_dir}
)
