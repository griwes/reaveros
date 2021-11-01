reaveros_add_aggregate_targets(loaders)

if ("uefi" IN_LIST REAVEROS_LOADERS)
    reaveros_add_aggregate_targets(loaders-uefi)
endif()

reaveros_automatic_components(loader loaders)
