cmake_minimum_required(VERSION 3.18)

project(reaveros-service-logger CXX)

add_executable(logger
    main.cpp
)

set_target_properties(logger
    PROPERTIES
        SUFFIX ".srv"
)

target_link_libraries(logger
    vdso
)

install(TARGETS logger
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
