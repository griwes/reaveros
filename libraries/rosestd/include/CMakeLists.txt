set(includes
    climits
    cstddef
    cstdint
    cstring
    cwchar

    atomic
    charconv
    chrono
    compare
    concepts
    format
    iterator
    memory
    mutex
    new
    optional
    ratio
    string_view
    system_error
    type_traits
    utility
    version

    __config
    __char_traits
)

if (NOT REAVEROS_ENABLE_UNIT_TESTS)
    set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/include)
else()
    set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/include/rosestd)
endif()

install(FILES ${includes}
    DESTINATION ${install_dir}
)
