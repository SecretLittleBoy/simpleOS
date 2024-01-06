.extern start_kernel

    .section .text.init
    .globl _start
_start:
    # set sp = VA2PA(boot_stack_top)
    la t0, PA2VA_OFFSET
    la sp, boot_stack_top # boot_stack_top是虚拟地址
    sub sp, sp, t0 # sp = sp - PA2VA_OFFSET

    jal ra, setup_vm
    jal ra, relocate
    jal ra, mm_init
    jal ra, setup_vm_final
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
    # sstatus ( Supervisor Status Register )中存在一个 SIE ( Supervisor Interrupt Enable ) 比特位
    # 当该比特位设置为 1 时，会响应所有的 S 态 trap， 否则将会禁用所有 S 态 trap。
    # csrr t0, sstatus
    # ori t0, t0, 0x2
    # csrw sstatus, t0

    jal x0, start_kernel

relocate:
    la t0, PA2VA_OFFSET
    add ra, ra, t0 # set ra = ra + PA2VA_OFFSET
    add sp, sp, t0 # set sp = sp + PA2VA_OFFSET

    # set satp with early_pgtbl
    la t1, early_pgtbl # load early_pgtbl address to t1
    sub t1, t1, t0     # t1 = early_pgtbl - PA2VA_OFFSET
    srli t1, t1, 12    # t1 = (early_pgtbl - PA2VA_OFFSET) >> 12 ; 顶级页表的物理页号。我们的物理页的大小为 4KB， PA >> 12 == PPN。
    li t0, 8     # MODE field = 8
    slli t0, t0, 60    # t0 = 0x8 << 60 #63-60字段mode为8
    or t1, t1, t0
    csrw satp, t1

    sfence.vma zero, zero //sfence.vma 指令用于刷新 TLB
    fence.i //fence.i 指令用于刷新 icache

    ret

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 0x1000*4 # <-- change to your stack size == PGSIZE

    .globl boot_stack_top
boot_stack_top: