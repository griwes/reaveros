file(GLOB sources *.cpp)

add_library(rosestd STATIC
    ${sources}
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

install(TARGETS rosestd
    DESTINATION ${install_dir}
)
