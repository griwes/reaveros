function(_reaveros_add_initrd_image_target architecture)
    set(_working_path ${REAVEROS_BINARY_DIR}/images/initrd-${architecture})
    set(_target_path ${REAVEROS_BINARY_DIR}/install/images/initrd-${architecture}.img)

    file(MAKE_DIRECTORY ${REAVEROS_BINARY_DIR}/install/images)
    file(MAKE_DIRECTORY ${_working_path})

    set(_bootinit ${REAVEROS_BINARY_DIR}/install/userspace/bootinit/${architecture}/bootinit)

    add_custom_command(OUTPUT ${_target_path} "always rebuilt"
        DEPENDS
            userspace-bootinit-${architecture}-install
            # all-${architecture}-userspace-services-core
            # all-${architecture}-userspace-services-storage
            # all-${architecture}-userspace-services-filesystem

        COMMAND rm -rf ${_working_path}
        COMMAND mkdir ${_working_path}

        COMMAND echo "00000000: 4d44 494e 4954 5244 0000 0000 0000 0000" > ${_working_path}/image.hex
        COMMAND echo -n "00000010: " >> ${_working_path}/image.hex
        COMMAND wc -c ${_bootinit} | awk \'{ printf \"%016x\", $$1 }\' >> ${_working_path}/image.hex
        COMMAND echo " 0000 0000 0000 0000" >> ${_working_path}/image.hex
        COMMAND xxd -o 0x1000 ${_bootinit} >> ${_working_path}/image.hex

        COMMAND xxd -r ${_working_path}/image.hex ${_working_path}/image.img

        COMMAND echo -n "00000008: " > ${_working_path}/patch-size.hex
        COMMAND wc -c ${_working_path}/image.img | awk \'{ printf \"%016x\", $$1 }\' >> ${_working_path}/patch-size.hex
        COMMAND xxd -r ${_working_path}/patch-size.hex ${_working_path}/image.img

        COMMAND echo -n "00000fff: " > ${_working_path}/patch-checksum.hex
        COMMAND xxd -p -c 1 ${_working_path}/image.img | awk -n \'{ sum = (sum + strtonum(\"0x\" $$1)) % 256 } END { printf \"%x\", (256 - sum) % 256 }\' >> ${_working_path}/patch-checksum.hex

        COMMAND cp ${_working_path}/image.img ${_working_path}/bak
        COMMAND xxd -r ${_working_path}/patch-checksum.hex ${_working_path}/image.img

        COMMAND cp ${_working_path}/image.img ${_target_path}
    )

    add_custom_target(image-initrd-${architecture}
        DEPENDS ${_target_path}
    )

    reaveros_register_target(image-initrd-${architecture} ${architecture} images initrd)
endfunction()

reaveros_add_aggregate_targets(images-initrd)

foreach (architecture IN LISTS REAVEROS_ARCHITECTURES)
    _reaveros_add_initrd_image_target(${architecture})
endforeach()

