file(GLOB sources *.cpp)

add_definitions(-g)

if (REAVEROS_IS_FREESTANDING)
    set(_library_type STATIC)
else()
    set(_library_type SHARED)
endif()

add_library(elf ${_library_type}
    ${sources}
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/lib)

install(TARGETS elf
    DESTINATION ${install_dir}
)
