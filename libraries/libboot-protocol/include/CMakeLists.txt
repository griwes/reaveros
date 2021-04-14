set(includes
    boot-constants.h
    boot-memmap.h
    boot-video.h
)

set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/include)

install(FILES ${includes}
    DESTINATION ${install_dir}
)
