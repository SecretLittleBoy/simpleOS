.extern start_kernel

    .section .text.init
    .globl _start
_start:

    la sp, boot_stack_top

    jal ra, mm_init
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

    .section .bss.stack
    .globl boot_stack
boot_stack:
    .space 1024*4 # <-- change to your stack size == PGSIZE

    .globl boot_stack_top
boot_stack_top: