    .section .text.entry
    .align 2
    .globl _traps 
_traps:
    #S-mode -> S-mode 不需要切换栈
    #U-mode -> S-mode 需要切换栈
    csrr t0,sstatus
    andi t0,t0,0x100
    bne t0,x0,SKIP_SWITCH_STACK_0    #SPP != 0
    csrr t0,sscratch
    csrw sscratch,sp
    add sp,t0,x0

SKIP_SWITCH_STACK_0:
    # 1. save 32 registers and sepc to stack
    addi sp, sp, -272
    sd x1, 0(sp)
    sd x2, 8(sp)
    sd x3, 16(sp)
    sd x4, 24(sp)
    sd x5, 32(sp)
    sd x6, 40(sp)
    sd x7, 48(sp)
    sd x8, 56(sp)
    sd x9, 64(sp)
    sd x10, 72(sp)
    sd x11, 80(sp)
    sd x12, 88(sp)
    sd x13, 96(sp)
    sd x14, 104(sp)
    sd x15, 112(sp)
    sd x16, 120(sp)
    sd x17, 128(sp)
    sd x18, 136(sp)
    sd x19, 144(sp)
    sd x20, 152(sp)
    sd x21, 160(sp)
    sd x22, 168(sp)
    sd x23, 176(sp)
    sd x24, 184(sp)
    sd x25, 192(sp)
    sd x26, 200(sp)
    sd x27, 208(sp)
    sd x28, 216(sp)
    sd x29, 224(sp)
    sd x30, 232(sp)
    sd x31, 240(sp)
    csrr a1, sepc    # a1 = sepc
    sd    a1,  248(sp)
    csrr t0, sstatus  # t0 = sscratch
    sd    t0, 256(sp)
    csrr t0, sscratch
    sd t0, 264(sp)

    # 2. call trap_handler
    //RISC-V的约定是，函数使用a0到a7这八个寄存器来传递输入参数，其中a0是最左边的参数，a7是最右边的参数。
    //函数使用a0寄存器来返回输出值。如果函数有多于八个的输入参数，那么多余的参数会被放在栈上。
    csrr a0, scause # scause ( Supervisor Cause Register ), 会记录 trap 发生的原因，还会记录该 trap 是 Interrupt 还是 Exception。
    csrr a1, sepc # sepc ( Supervisor Exception Program Counter ), 会记录触发 exception 的那条指令的地址。
    add a2, sp, x0
    jal ra, trap_handler

    ld t0, 264(sp)
    csrw sscratch, t0
    ld    t0, 248(sp)
    csrw  sepc, t0
    ld    t0, 256(sp)
    csrw  sstatus, t0
    # 3. restore sepc and 32 registers (x2(sp) should be restore last) from stack
    ld x1, 0(sp)
    ld x3, 16(sp)
    ld x4, 24(sp)
    ld x5, 32(sp)
    ld x6, 40(sp)
    ld x7, 48(sp)
    ld x8, 56(sp)
    ld x9, 64(sp)
    ld x10, 72(sp)
    ld x11, 80(sp)
    ld x12, 88(sp)
    ld x13, 96(sp)
    ld x14, 104(sp)
    ld x15, 112(sp)
    ld x16, 120(sp)
    ld x17, 128(sp)
    ld x18, 136(sp)
    ld x19, 144(sp)
    ld x20, 152(sp)
    ld x21, 160(sp)
    ld x22, 168(sp)
    ld x23, 176(sp)
    ld x24, 184(sp)
    ld x25, 192(sp)
    ld x26, 200(sp)
    ld x27, 208(sp)
    ld x28, 216(sp)
    ld x29, 224(sp)
    ld x30, 232(sp)
    ld x31, 240(sp)
    ld x2, 8(sp) # x2 是 sp，所以要最后恢复
    addi sp, sp, 272

    #S-mode -> S-mode 不需要切换栈
    #S-mode -> U-mode 需要切换栈
    csrr t0,sstatus
    andi t0,t0,0x100
    bne t0,x0,SKIP_SWITCH_STACK_1    #SPP != 0
    csrr t0,sscratch
    csrw sscratch,sp
    add sp,t0,x0
SKIP_SWITCH_STACK_1:
    # 4. return from trap
    sret
    #sret是一个真正的指令，用于从监管者模式下的陷阱（异常或中断）返回。
    #它会将sepc寄存器的值跳转到pc寄存器，并根据sstatus寄存器的SPP位恢复执行模式（用户模式或监管者模式）。
    
    #ret是一个伪指令，它实际上是一个jalr指令，用于从普通函数返回。它会将ra寄存器的值跳转到pc寄存器，继续执行调用者的指令流。




    .extern dummy
    .global __dummy
__dummy:
    # Swap sp and sscratch 
    mv t0, sp               # t0 = sp
    csrr sp, sscratch       # sp = ssc
    csrw sscratch, t0       # ssc = t0
    
    # la t0, dummy #todo why delete this line
    # csrw sepc, t0 #todo why delete this line
    sret # exit from trap, and jump to dummy


#void __switch_to(struct task_struct *prev, struct task_struct *next);
#保存当前线程的ra，sp，s0~s11到当前线程的 thread_struct 中
#将下一个线程的 thread_struct 中的相关数据载入到ra，sp，s0~s11中
    .globl __switch_to
__switch_to:
    sd ra, 0(a0)
    sd sp, 8(a0)
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    csrr t0, sepc
    sd t0, 112(a0)

    csrr t0, sstatus
    sd t0, 120(a0)

    csrr t0, sscratch
    sd t0, 128(a0)


    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)

    ld t0, 112(a1)
    csrw sepc, t0

    ld t0, 120(a1)
    csrw sstatus, t0

    ld t0, 128(a1)
    csrw sscratch, t0

    csrw satp, a2                   # set page table

    sfence.vma zero, zero

    ret