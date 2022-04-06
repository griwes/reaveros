set(install_dir ${CMAKE_INSTALL_PREFIX}/usr/include)

install(DIRECTORY elf
    DESTINATION ${install_dir}
)
