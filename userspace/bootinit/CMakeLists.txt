cmake_minimum_required(VERSION 3.20)
project(reaveros-bootinit)

add_executable(bootinit-elf
    main.cpp
)

set(linker_script ${CMAKE_CURRENT_SOURCE_DIR}/bootinit.lds)

set_target_properties(bootinit-elf
    PROPERTIES
        COMPILE_FLAGS "-g -mcmodel=kernel -DREAVEROS_KERNEL_SOURCE_ROOT='\"${CMAKE_SOURCE_DIR}\"'"
        LINK_FLAGS "-T ${linker_script} -static -Map=bootinit.map -nostdlib"
        LINK_DEPENDS ${linker_script}
)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/bootinit
    DEPENDS bootinit-elf

    COMMAND
        ${CMAKE_OBJCOPY} -O binary -j .text -j .data
            $<TARGET_FILE:bootinit-elf>
            ${CMAKE_BINARY_DIR}/bootinit
)

add_custom_target(bootinit-flat ALL DEPENDS ${CMAKE_BINARY_DIR}/bootinit)

install(FILES ${CMAKE_BINARY_DIR}/bootinit
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
