file(GLOB sources *.cpp)

add_definitions(-g)

add_library(archive STATIC
    ${sources}
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

install(TARGETS archive
    DESTINATION ${install_dir}
)
