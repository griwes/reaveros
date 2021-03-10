set(includes
    climits
    cstddef
    cstdint
    cstring
    cwchar

    concepts
    iterator
    memory
    string_view
    type_traits
    utility

    __char_traits
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/include)

install(FILES ${includes}
    DESTINATION ${install_dir}
)
