.extern start_kernel

    .section .text.init
    .globl _start
_start:

    #lui sp, 0x40104
    #slli sp, sp, 1 # sp = 0x80208000 # 8000 是因为栈空间4000，vmlinux.lds中描述的.text等段总共4000，加起来8000
    
    # get 0xffffffdf80000000 (PA2VA_OFFSET) in t0
    addi t0, x0, 1  # t0 = 0x00000001
    slli t0, t0, 31 # t0 = 0x80000000
    lui t1, 0xfffff    # t1 = 0xfffff000
    addi t1, t1, 0x7ef # t1 = 0xfffff7ef
    addi t1, t1, 0x7f0 # t1 = 0xffffffdf
    slli t1, t1, 31
    slli t1, t1, 1     # t1 = 0xffffffdf00000000
    add t0, t0, t1    # t0 = 0xffffffdf80000000 = PA2VA_OFFSET
    la sp, boot_stack_top # boot_stack_top是虚拟地址
    sub sp, sp, t0 # sp = sp - PA2VA_OFFSET

    jal ra, setup_vm
    jal ra, relocate

    jal ra, mm_init
    /*
    mm_init 为什么不会把kernel的代码init清空掉？ 在relocate开启虚拟地址后，kernel的地址就变成虚拟地址。_ekernel = 物理地址_ekernel + PA2VA_OFFSET
    在 RISC-V Linux Kernel Space 中有一段虚拟地址空间中的区域被称为 direct mapping area，
    为了方便访问内存，内核会预先把所有物理内存都映射至这一块区域，这种映射也被称为 linear mapping，
    因为该映射方式就是在物理地址上添加一个偏移，使得 VA = PA + PA2VA_OFFSET。
    在 RISC-V Linux Kernel 中这一段区域为 0xffffffe000000000 ~ 0xffffffff00000000，共 124 GB 。
    */

    jal ra, setup_vm_final # set up real virtual memory

    jal ra, task_init

    # set stvec = _traps 中断向量表基址
    la t0, _traps
    csrw stvec, t0

    # set sie[STIE] = 1 开启时钟中断
    # Interrupt 被划分为三类 Software Interrupt， Timer Interrupt， External Interrupt。
    # 在开启了 sstatus[SIE]之后，系统会根据 sie 中的相关比特位来决定是否对该 Interrupt 进行处理。
    csrr t0, sie
    ori t0, t0, 0x20
    csrw sie, t0

    # set first time interrupt
    jal ra, clock_set_next_event

    # set sstatus[SIE] = 1
    #sstatus ( Supervisor Status Register )中存在一个 SIE ( Supervisor Interrupt Enable ) 比特位
    #当该比特位设置为 1 时，会响应所有的 S 态 trap， 否则将会禁用所有 S 态 trap。
    csrr t0, sstatus
    ori t0, t0, 0x2
    csrw sstatus, t0

    jal x0, start_kernel


    relocate:
    # set ra = ra + PA2VA_OFFSET
    # set sp = sp + PA2VA_OFFSET (If you have set the sp before)

    ###################### 
    #   YOUR CODE HERE   #
    ######################
    # get 0xffffffdf80000000 (PA2VA_OFFSET) in t0
    addi t0, x0, 1  # t0 = 0x00000001
    slli t0, t0, 31 # t0 = 0x80000000
    lui t1, 0xfffff    # t1 = 0xfffff000
    addi t1, t1, 0x7ef # t1 = 0xfffff7ef
    addi t1, t1, 0x7f0 # t1 = 0xffffffdf
    slli t1, t1, 31
    slli t1, t1, 1     # t1 = 0xffffffdf00000000
    add t0, t0, t1    # t0 = 0xffffffdf80000000 = PA2VA_OFFSET

    add ra, ra, t0 # set ra = ra + PA2VA_OFFSET
    add sp, sp, t0 # set sp = sp + PA2VA_OFFSET

    # set satp with early_pgtbl
    la t2, early_pgtbl # load early_pgtbl address to t2
    sub t2, t2, t0    # t2 = early_pgtbl - PA2VA_OFFSET
    add t1, x0, t2    # t1 = early_pgtbl - PA2VA_OFFSET
    srli t1, t1, 12   # t1 = (early_pgtbl - PA2VA_OFFSET) >> 12 ; 顶级页表的物理页号。我们的物理页的大小为 4KB， PA >> 12 == PPN。
    # MODE field = 8
    addi t0, x0, 8
    slli t0, t0, 20
    slli t0, t0, 20
    slli t0, t0, 20   # t0 = 0x8 << 60
    or t1, t1, t0
    csrw satp, t1
    ###################### 
    #   YOUR CODE HERE   #
    ######################

    # flush tlb
    sfence.vma zero, zero //sfence.vma 指令用于刷新 TLB

    # flush icache
    fence.i //fence.i 指令用于刷新 icache

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 0x1000*4 # <-- change to your stack size == PGSIZE

    .globl boot_stack_top
boot_stack_top: