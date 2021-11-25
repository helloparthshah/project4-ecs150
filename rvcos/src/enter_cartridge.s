.section .init, "ax"
.global enter_cartridge, switch_context, call_on_other_gp
.extern saved_sp
enter_cartridge:
    addi    sp,sp,-12
    la      a5,saved_sp
    sw      sp,0(a5)
    sw      s0,8(sp)
    sw      s1,4(sp)
    sw      ra,0(sp)
    li      a5,0x4000001C
    lw	    a5,0(a5)
    andi	a5,a5,-4
    jalr	a5
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la      a5,saved_sp
    lw      sp,0(a5)
    lw      s0,8(sp)
    lw      s1,4(sp)
    lw      ra,0(sp)
    addi    sp,sp,12
    ret

switch_context:
    addi    sp,sp,-52
    sw      ra,48(sp)
    sw      tp,44(sp)
    sw      t0,40(sp)
    sw      t1,36(sp)
    sw      t2,32(sp)
    sw      s0,28(sp)
    sw      s1,24(sp)
    sw      a0,20(sp)
    sw      a1,16(sp)
    sw      a2,12(sp)
    sw      a3,8(sp)
    sw      a4,4(sp)
    sw      a5,0(sp)
    sw      sp,0(a0)
    mv      sp,a1
    lw      ra,48(sp)
    lw      tp,44(sp)
    lw      t0,40(sp)
    lw      t1,36(sp)
    lw      t2,32(sp)
    lw      s0,28(sp)
    lw      s1,24(sp)
    lw      a0,20(sp)
    lw      a1,16(sp)
    lw      a2,12(sp)
    lw      a3,8(sp)
    lw      a4,4(sp)
    lw      a5,0(sp)
    addi    sp,sp,52
    ret

call_on_other_gp:
    addi    sp,sp,-4
    sw      ra,0(sp)
    mv      gp,a2
    jalr    a1
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop
    lw      ra,0(sp)
    addi    sp,sp,4
    ret
