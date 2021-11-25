.section .init, "ax"
.global _start
_start:
    .cfi_startproc
    .cfi_undefined ra
    .option push
    .option norelax
    la gp, __global_pointer$ # init the global pointer
    .option pop
    la sp, __stack_top
    add s0, sp, zero
    la  a5, _interrupt_handler # init interrupt handler
    csrw mtvec, a5 # control status handler points to the interrupt handler if only 1
    jal ra, init
    nop
    jal zero, main # Calls main
    .cfi_endproc
    .end
