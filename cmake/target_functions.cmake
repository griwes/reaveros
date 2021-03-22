function(reaveros_add_aggregate_targets _suffix)
    if (NOT "${_suffix}" STREQUAL "")
        add_custom_target(all-${_suffix})
        set(_suffix "-${_suffix}")
    endif()

    foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
        add_custom_target(all-${toolchain}${_suffix})
    endforeach()

    foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
        add_custom_target(all-${architecture}${_suffix})

        foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
            add_custom_target(all-${toolchain}-${architecture}${_suffix})
        endforeach()
    endforeach()

    foreach (mode IN ITEMS kernel user)
        add_custom_target(all-${mode}${_suffix})

        foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
            add_custom_target(all-${toolchain}-${mode}${_suffix})
        endforeach()

        foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
            add_custom_target(all-${architecture}-${mode}${_suffix})

            foreach (toolchain IN LISTS REAVEROS_TOOLCHAINS)
                add_custom_target(all-${toolchain}-${architecture}-${mode}${_suffix})
            endforeach()
        endforeach()
    endforeach()
endfunction()

function(_reaveros_register_target_impl _target _head _tail)
    list(LENGTH _tail _tail_length)
    math(EXPR _tail_length_prev "${_tail_length} - 1")

    foreach (_index RANGE 0 ${_tail_length_prev})
        list(GET _tail ${_index} _current_element)
        set(_full_list ${_head} ${_current_element})

        string(REPLACE ";" "-" _aggregate_target "${_full_list}")
        if (TARGET all-${_aggregate_target})
            #message("add_dependencies(all-${_aggregate_target} ${_target})")
            add_dependencies(all-${_aggregate_target} ${_target})
            #else()
            #message("skipping add_dependencies(all-${_aggregate_target} ${_target})")
        endif()

        math(EXPR _index_next "${_index} + 1")
        if (${_index_next} LESS ${_tail_length})
            list(SUBLIST _tail ${_index_next} -1 _current_tail)

            _reaveros_register_target_impl(${_target} "${_full_list}" "${_current_tail}")
        endif()
    endforeach()
endfunction()

function(reaveros_register_target _target)
    _reaveros_register_target_impl(${_target} "" "${ARGN}")
endfunction()

function(reaveros_add_ep_prune_target external_project)
    ExternalProject_Add_Step(${external_project}
        prune
        COMMAND rm -rf <SOURCE_DIR> <BINARY_DIR>
        DEPENDERS mkdir download
        EXCLUDE_FROM_MAIN TRUE
        INDEPENDENT TRUE
    )
    ExternalProject_Add_StepTargets(${external_project} prune)

    add_dependencies(all-toolchains-prune
        ${external_project}-prune
    )
endfunction()

function(reaveros_add_ep_fetch_tag_target external_project tag)
    ExternalProject_Add_Step(${external_project}
        fetch-tag
        COMMAND ${GIT_EXECUTABLE} fetch origin ${tag} --depth=1
        WORKING_DIRECTORY <SOURCE_DIR>
        DEPENDEES download
        DEPENDERS update configure build
        EXCLUDE_FROM_MAIN TRUE
        INDEPENDENT TRUE
    )
endfunction()
