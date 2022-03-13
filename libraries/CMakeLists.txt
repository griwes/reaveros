reaveros_add_aggregate_targets(libraries)

set(libraries
    rosestd

    boot-protocol

    archive
)

foreach (library IN LISTS libraries)
    reaveros_include_component(${library} library libraries)
endforeach()
