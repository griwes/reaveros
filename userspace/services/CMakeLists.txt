reaveros_add_aggregate_targets(userspace-services)

set(services
    vasmgr
)

foreach (service IN LISTS services)
    reaveros_include_component(${service} userspace-service userspace services)
endforeach()
