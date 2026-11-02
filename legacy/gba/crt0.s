/*
 * crt0.s - GBA startup code
 */

.arm
.syntax unified

.section .header, "ax"
.global _header
_header:
    b _start
    .space 0xBC  /* Space for Nintendo logo, game title, and header fields */

.section .crt0, "ax"
.global _start
_start:
    /* Set IRQ stack pointer */
    mov r0, #0x12       /* IRQ mode */
    msr cpsr_c, r0
    ldr sp, =__sp_irq

    /* Set System/User stack pointer */
    mov r0, #0x1F       /* System mode */
    msr cpsr_c, r0
    ldr sp, =__sp_usr

    /* Copy .data from ROM to EWRAM */
    ldr r0, =__data_load
    ldr r1, =__data_start
    ldr r2, =__data_end
copy_data_loop:
    cmp r1, r2
    bge copy_data_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data_loop
copy_data_done:

    /* Zero out .bss in EWRAM */
    ldr r1, =__bss_start
    ldr r2, =__bss_end
    mov r3, #0
zero_bss_loop:
    cmp r1, r2
    bge zero_bss_done
    str r3, [r1], #4
    b zero_bss_loop
zero_bss_done:

    /* Call C++ main function */
    ldr r0, =main
    bx r0

hang:
    b hang
