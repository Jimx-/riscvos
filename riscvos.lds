OUTPUT_ARCH(riscv)
ENTRY (_start)

_VIR_BASE = 0xffffffe000000000;

SECTIONS{
    . = _VIR_BASE;
    _start = .;

    . = ALIGN(4096);
    .text : {
        _text = .;
         *(.head)
         *(.text)
        _etext = .;
    }
    . = ALIGN(4096);
    .rodata : {
        *(.rodata)
    }
    . = ALIGN(4096);
    .data : {
        __global_pointer$ = . + 0x800;
        _data = .;
         *(.data)
        _edata = .;
    }
    . = ALIGN(4096);
    .bss : {
        _bss = .;
         *(COMMON)
         *(.bss)
        _ebss = .;
    }
    _KERN_SIZE = . - _VIR_BASE;
    _end = .;
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.gnu.build-id)
    }
}
