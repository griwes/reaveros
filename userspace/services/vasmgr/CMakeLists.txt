cmake_minimum_required(VERSION 3.18)

project(reaveros-service-vasmgr CXX)

add_executable(vasmgr
    main.cpp
)

set_target_properties(vasmgr
    PROPERTIES
        SUFFIX ".srv"
)

install(TARGETS vasmgr
    DESTINATION ${CMAKE_INSTALL_PREFIX}
)
