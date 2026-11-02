/*
 * gba.ld - Linker Script for Game Boy Advance with Clang / LLD
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)

MEMORY {
    ewram (w!x) : ORIGIN = 0x02000000, LENGTH = 256K
    iwram (w!x) : ORIGIN = 0x03000000, LENGTH = 32K
    rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
}

__sp_usr = 0x03007F00;
__sp_irq = 0x03007FA0;

SECTIONS {
    . = 0x08000000;

    .header : {
        KEEP(*(.header))
        . = ALIGN(4);
    } > rom

    .text : {
        *(.crt0)
        *(.text .text.*)
        *(.gnu.linkonce.t.*)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.r.*)
        . = ALIGN(4);
    } > rom

    __text_end = .;

    .data : AT (__text_end) {
        __data_start = .;
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        . = ALIGN(4);
        __data_end = .;
    } > ewram

    __data_load = LOADADDR(.data);

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > ewram

    .iwram (NOLOAD) : {
        __iwram_start = .;
        *(.iwram .iwram.*)
        . = ALIGN(4);
        __iwram_end = .;
    } > iwram

    /DISCARD/ : {
        *(.eh_frame)
        *(.comment)
        *(.ARM.exidx*)
    }
}
