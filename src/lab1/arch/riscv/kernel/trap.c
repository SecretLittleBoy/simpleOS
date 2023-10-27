// trap.c

/*
scause ( Supervisor Cause Register ), 会记录 trap 发生的原因，还会记录该 trap 是 Interrupt 还是 Exception。
sepc ( Supervisor Exception Program Counter ), 会记录触发 exception 的那条指令的地址。
*/

void trap_handler(unsigned long scause, unsigned long sepc){
    // 通过 `scause` 判断trap类型
    // 如果是interrupt 判断是否是timer interrupt
    // 如果是timer interrupt 则打印输出相关信息, 并通过 `clock_set_next_event()` 设置下一次时钟中断
    // `clock_set_next_event()` 见 4.3.4 节
    // 其他interrupt / exception 可以直接忽略

    // YOUR CODE HERE
    unsigned long interrupt_sig = 0x8000000000000000;
    if (scause & interrupt_sig){ // it's interrupt
        scause = scause - interrupt_sig;
        unsigned long timer_int = 0x5;
        if (!(scause ^ timer_int)){ // it's Supervisor timer interrupt
            printk("Supervisor Mode Timer Interrupt\n");
            clock_set_next_event();
            return;
        }
    }
    
    printk("other interrupt or exception\n");
}