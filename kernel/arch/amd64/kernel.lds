ENTRY(kernel_main)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 0xFFFFFFFF80000000;

    .text (0xFFFFFFFF80000000) :
    {
        *(.reaveros_entry)
        KEEP(*(.text))
        KEEP(*(.text.*))
        *(.gnu.linkonce.t*)
    }

    .data :
    {
        . = ALIGN(0x1000);
        *(.rodata*)
        *(.gnu.linkonce.r*)
        KEEP(*(.init))

        . = ALIGN(8);
        __start_ctors = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.ctors .init_array))
        __end_ctors = .;

        . = ALIGN(0x1000);
        *(.data)
        *(.data.*)
        *(.gnu.linkonce.d*)

        . = ALIGN(0x1000);
        *(COMMON)
        *(.bss)
        *(.gnu.linkonce.b*)

        . = ALIGN(0x1000);
        KEEP(*(.bootinit))

        . = ALIGN(0x1000);
        KEEP(*(.vdso))

        . = ALIGN(0x1000);
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.dynsym)
        *(.dynstr)
        *(.rela.dyn)
        *(.hash)
        *(.gnu.hash*)
        *(.dynamic)
    }
}

