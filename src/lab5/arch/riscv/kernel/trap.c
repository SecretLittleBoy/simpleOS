#include "proc.h"
#include "syscall.h"
#define INTERRUPT_SIG 0x8000000000000000
#define TIMER_INTERRUPT_SIG 0x5
#define ECALL_FROM_USER 0x8
#define INSTRUCTION_PAGE_FAULT 0xc
#define LOAD_PAGE_FAULT 0xd
#define STORE_PAGE_FAULT 0xf

#define SYS_WRITE 64
#define SYS_GETPID 172
/*
scause ( Supervisor Cause Register ), 会记录 trap 发生的原因，还会记录该 trap 是 Interrupt 还是 Exception。
sepc ( Supervisor Exception Program Counter ), 会记录触发 exception 的那条指令的地址。
当触发 page fault 时，stval 寄存器被被硬件自动设置为该出错的 VA 地址

ecall ( Environment Call )，当我们在 S 态执行这条指令时，会触发一个 ecall-from-s-mode-exception，从而进入 M Mode 下的处理流程( 如设置定时器等 )；
当我们在 U 态执行这条指令时，会触发一个 ecall-from-u-mode-exception，从而进入 S Mode 下的处理流程 ( 常用来进行系统调用 )。
*/
void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    if (scause & INTERRUPT_SIG) { // it's interrupt
        scause = scause - INTERRUPT_SIG;
        if (!(scause ^ TIMER_INTERRUPT_SIG)) { // it's Supervisor timer interrupt
            // printk("Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
            return;
        }
    } else if (scause & ECALL_FROM_USER) { // ecall-from-user-mode
        //  printk("ecall from user mode\n");
        // 系统调用参数使用 a0 - a5 ，系统调用号使用 a7 ， 系统调用的返回值会被保存到 a0, a1 中。
        if (regs->a7 == SYS_WRITE) {
            printk("write to fd %d, %d bytes\n", regs->a0, regs->a2);
            regs->a0 = sys_write(regs->a0, (const char *)regs->a1, regs->a2); // 返回值放入a0
        } else if (regs->a7 == SYS_GETPID) {
            regs->a0 = sys_getpid();
            printk("getpid: %d\n", regs->a0);
        }
        regs->sepc += 4;
        return;
    } else if (scause == INSTRUCTION_PAGE_FAULT || scause == LOAD_PAGE_FAULT || scause == STORE_PAGE_FAULT) {
        printk("[S] Page Fault at va: %lx, sepc: %lx, scause: %lx\n", regs->stval, sepc, scause);
        
    } else {
        printk("Unhandled exception! scause: %llx\n", scause);
    }
}