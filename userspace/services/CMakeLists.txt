reaveros_add_aggregate_targets(userspace-services)

set(services
    logger
)

foreach (service IN LISTS services)
    reaveros_include_component(${service} userspace-service userspace services)
endforeach()
