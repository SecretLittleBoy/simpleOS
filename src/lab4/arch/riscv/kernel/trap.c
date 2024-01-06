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
*/

void trap_handler(unsigned long scause, unsigned long sepc, struct pt_regs *regs) {
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他interrupt / exception 可以直接忽略

    if (scause & INTERRUPT_SIG) { // it's interrupt
        scause = scause - INTERRUPT_SIG;
        if (!(scause ^ TIMER_INTERRUPT_SIG)) { // it's Supervisor timer interrupt
            // printk("Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            do_timer();
            return;
        }
    } else if (scause & ECALL_FROM_USER) { // ecall-from-user-mode
        // printk("ecall from user mode\n");
        if (regs->reg[16] == SYS_WRITE) { // a7
            printk("write to fd %d, %d bytes\n", regs->reg[9], regs->reg[11]);
            regs->reg[9] = sys_write(regs->reg[9], (const char *)regs->reg[10], regs->reg[11]);
            //  a0                       a0            a1            a2
        }

        /* 172 号系统调用 sys_getpid() 该调用从current中获取当前的pid放入a0中返回，无参数。
         *（ 具体见 user/getpid.c ） */
        else if (regs->reg[16] == SYS_GETPID) {// a7
            regs->reg[9] = sys_getpid();      // a0
            printk("getpid: %d\n", regs->reg[9]);
        }
        regs->sepc += 4;
        return;
    } else {
        printk("Unhandled exception! scause: %llx\n", scause);
    }
}