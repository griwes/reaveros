file(GLOB sources *.cpp)

if (REAVEROS_IS_FREESTANDING)
    set(_library_type STATIC)
else()
    set(_library_type SHARED)
endif()

set(CMAKE_SHARED_LINKER_FLAGS -nostdlib)

add_library(rosestd ${_library_type}
    ${sources}
)

set_target_properties(rosestd
    PROPERTIES
        COMPILE_FLAGS "-g"
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

install(TARGETS rosestd
    DESTINATION ${install_dir}
)
