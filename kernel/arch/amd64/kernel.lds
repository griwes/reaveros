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

    .init :
    {
        KEEP(*(.init))
    }

    .rodata : ALIGN (0x1000)
    {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    }

    .init_array : ALIGN (0x1000)
    {
        __start_ctors = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.ctors .init_array))
        __end_ctors = .;
    }

    .data : ALIGN (0x1000)
    {
        *(.data)
        *(.gnu.linkonce.d*)

        *(COMMON)
        *(.bss)
        *(.gnu.linkonce.b*)
    }

    __begin_scratch_space = .;
    . = . + 4 * 1024 * 1024;
    __end_scratch_space = .;

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

