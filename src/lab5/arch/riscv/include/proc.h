#include "types.h"
#include "stdint.h"
typedef unsigned long *pagetable_t;

#define NR_TASKS (1 + 3) // 用于控制 最大线程数量 （idle 线程 + 3 内核线程）

#define TASK_RUNNING 0 // 为了简化实验, 所有的线程都只有一种状态

/* 线程状态段数据结构 */
struct thread_struct {
    // 每个线程都有自己的用户栈和内核栈
    uint64 ra; // 返回地址. 初始化为 __dummy, 因为第一次切换到此线程时, 不用从此线程的内核栈中恢复此线程的上下文。之后的ra值指向switch_to的结尾，之后会跳转到_traps，从此线程的内核栈中恢复此线程的上下文。
    uint64 sp; // 此线程的内核栈指针。初始化为此线程的内核栈顶。
    uint64 s[12]; // 保存的寄存器
    uint64_t sepc, sstatus, sscratch; // sscratch保存用户态栈的栈顶，初始化为USER_END
};

/* 线程数据结构 */
struct task_struct {
    uint64 state;    // 线程状态
    uint64 counter;  // 运行剩余时间
    uint64 priority; // 运行优先级 1最低 10最高
    uint64 pid;      // 线程id

    struct thread_struct thread;

    pagetable_t pgd; // 页表
};

struct pt_regs {
    uint64 reg[31];
    uint64 sepc;
    uint64 sstatus;
    uint64 sscratch;
};

/* 线程初始化 创建 NR_TASKS 个线程 */
void task_init();

/* 在时钟中断处理中被调用 用于判断是否需要进行调度 */
void do_timer();

/* 调度程序 选择出下一个运行的线程 */
void schedule();

/* 线程切换入口函数*/
void switch_to(struct task_struct *next);

/* dummy funciton: 一个循环程序, 循环输出自己的 pid 以及一个自增的局部变量 */
void dummy();