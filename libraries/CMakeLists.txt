set(libraries
    rosert
    rosestd

    boot-protocol

    archive
    elf
)

foreach (library IN LISTS libraries)
    reaveros_include_component(${library} library libraries)
endforeach()
