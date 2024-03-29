#include "proc.h"
#include "syscall.h"
#define INTERRUPT_SIG 0x8000000000000000
#define TIMER_INTERRUPT_SIG 0x5
#define ECALL_FROM_USER 0x8

#define SYS_WRITE 64
#define SYS_GETPID 172
/*
scause ( Supervisor Cause Register ), 会记录 trap 发生的原因，还会记录该 trap 是 Interrupt 还是 Exception。
sepc ( Supervisor Exception Program Counter ), 会记录触发 exception 的那条指令的地址。

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
        if (regs->reg[16] == SYS_WRITE) { // regs->reg[16]: a7
            printk("write to fd %d, %d bytes\n", regs->reg[9], regs->reg[11]);
            regs->reg[9] = sys_write(regs->reg[9], (const char *)regs->reg[10], regs->reg[11]); // 返回值放入a0
            //  a0                       a0            a1            a2
        } else if (regs->reg[16] == SYS_GETPID) { // a7
            regs->reg[9] = sys_getpid();          // a0
            printk("getpid: %d\n", regs->reg[9]);
        }
        regs->sepc += 4;
        return;
    } else {
        printk("Unhandled exception! scause: %llx\n", scause);
    }
}