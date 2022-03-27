file(GLOB sources *.cpp)

add_definitions(-g)

if (REAVEROS_IS_FREESTANDING)
    set(_library_type STATIC)
else()
    set(_library_type SHARED)
endif()

add_library(rosestd ${_library_type}
    ${sources}
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

install(TARGETS rosestd
    DESTINATION ${install_dir}
)
