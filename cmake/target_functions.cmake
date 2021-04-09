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
    ExternalProject_Get_Property(${external_project} STAMP_DIR)

    get_property(has_git_tag TARGET ${external_project} PROPERTY _EP_GIT_TAG SET)
    if (has_git_tag)
        get_property(GIT_TAG TARGET ${external_project} PROPERTY _EP_GIT_TAG)
        file(GLOB force_download_stamps ${STAMP_DIR}/${external_project}-force-download-*)
        list(REMOVE_ITEM force_download_stamps ${STAMP_DIR}/${external_project}-force-download-${GIT_TAG})
    endif()

    file(TOUCH ${STAMP_DIR}/${external_project}-skip-update)
    file(TOUCH ${STAMP_DIR}/${external_project}-configure)
    file(TOUCH ${STAMP_DIR}/${external_project}-build)
    file(TOUCH ${STAMP_DIR}/${external_project}-install)

    set(_commands
        COMMAND rm -rf <SOURCE_DIR> <BINARY_DIR>
        COMMAND rm -rf ${force_download_stamps}
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-gitclone-lastrun.txt
        COMMAND touch ${STAMP_DIR}/${external_project}-set-to-tag
        COMMAND touch ${STAMP_DIR}/${external_project}-skip-update
        COMMAND touch ${STAMP_DIR}/${external_project}-configure
        COMMAND touch ${STAMP_DIR}/${external_project}-build
        COMMAND touch ${STAMP_DIR}/${external_project}-install
    )

    ExternalProject_Add_Step(${external_project}
        prune
        ${_commands}
        EXCLUDE_FROM_MAIN TRUE
        INDEPENDENT TRUE
    )
    ExternalProject_Add_StepTargets(${external_project} prune)

    add_dependencies(all-toolchains-prune
        ${external_project}-prune
    )
endfunction()

function(reaveros_add_ep_fetch_tag_target external_project)
    ExternalProject_Get_Property(${external_project} STAMP_DIR GIT_TAG)

    ExternalProject_Add_Step(${external_project}
        set-to-tag
        COMMAND ${GIT_EXECUTABLE} fetch origin ${GIT_TAG} --depth=1
        COMMAND ${GIT_EXECUTABLE} checkout ${GIT_TAG}
        WORKING_DIRECTORY <SOURCE_DIR>
        DEPENDEES download
        DEPENDERS update configure build
        EXCLUDE_FROM_MAIN TRUE
        INDEPENDENT TRUE
    )

    file(GLOB force_download_stamps ${STAMP_DIR}/${external_project}-force-download-*)
    list(REMOVE_ITEM force_download_stamps ${STAMP_DIR}/${external_project}-force-download-${GIT_TAG})

    ExternalProject_Add_Step(${external_project}
        force-download-${GIT_TAG}
        COMMAND rm -rf ${force_download_stamps}
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-mkdir
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-download
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-gitclone-lastrun.txt
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-configure
        DEPENDERS mkdir download update set-to-tag prune
        EXCLUDE_FROM_MAIN TRUE
        INDEPENDENT TRUE
    )

    add_custom_command(TARGET ${external_project}
        COMMAND touch ${STAMP_DIR}/${external_project}-set-to-tag
        COMMAND touch ${STAMP_DIR}/${external_project}-skip-update
        COMMAND touch ${STAMP_DIR}/${external_project}-configure
        COMMAND touch ${STAMP_DIR}/${external_project}-build
        COMMAND touch ${STAMP_DIR}/${external_project}-install
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-prune
    )
    add_custom_command(TARGET ${external_project}-install
        COMMAND touch ${STAMP_DIR}/${external_project}-set-to-tag
        COMMAND touch ${STAMP_DIR}/${external_project}-skip-update
        COMMAND touch ${STAMP_DIR}/${external_project}-configure
        COMMAND touch ${STAMP_DIR}/${external_project}-build
        COMMAND touch ${STAMP_DIR}/${external_project}-install
        COMMAND rm -rf ${STAMP_DIR}/${external_project}-prune
    )
endfunction()

