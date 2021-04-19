set(includes
    climits
    cstddef
    cstdint
    cstring
    cwchar

    charconv
    compare
    concepts
    format
    iterator
    memory
    string_view
    system_error
    type_traits
    utility

    __char_traits
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/include)

install(FILES ${includes}
    DESTINATION ${install_dir}
)
