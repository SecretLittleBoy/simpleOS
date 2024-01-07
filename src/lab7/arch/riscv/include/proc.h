#include "types.h"
#include "stdint.h"
typedef unsigned long *pagetable_t;

#define VM_X_MASK 0x0000000000000008
#define VM_W_MASK 0x0000000000000004
#define VM_R_MASK 0x0000000000000002
#define VM_ANONYM 0x0000000000000001

#define NR_TASKS (1 + 1) // 用于控制 最大线程数量 （idle 线程 + 1 内核线程）

#define TASK_RUNNING 0 // 为了简化实验, 所有的线程都只有一种状态

struct vm_area_struct {
    uint64_t vm_start; /* VMA 对应的用户态虚拟地址的开始 */
    uint64_t vm_end;   /* VMA 对应的用户态虚拟地址的结束 */
    uint64_t vm_flags; /* VMA 对应的 flags */

    /* uint64_t file_offset_on_disk */ /* 原本需要记录对应的文件在磁盘上的位置，
                               但是我们只有一个文件 uapp，所以暂时不需要记录 */

    uint64_t vm_content_offset_in_file; /* 如果对应了一个文件，
         那么这块 VMA 起始地址对应的文件内容相对文件起始位置的偏移量，
                           也就是 ELF 中各段的 p_offset 值 */

    uint64_t vm_content_size_in_file; /* 对应的文件内容的长度。
                                       思考为什么还需要这个域?
                                       和 (vm_end-vm_start)
                                       一比，不是冗余了吗? */
};

/* 线程状态段数据结构 */
struct thread_struct {
    // 每个线程都有自己的用户栈和内核栈
    uint64 ra;                        // 返回地址. 初始化为 __dummy, 因为第一次切换到此线程时, 不用从此线程的内核栈中恢复此线程的上下文。之后的ra值指向switch_to的结尾，之后会跳转到_traps，从此线程的内核栈中恢复此线程的上下文。
    uint64 sp;                        // 此线程的内核栈指针。初始化为此线程的内核栈顶。
    uint64 s[12];                     // 保存的寄存器
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

    uint64_t vma_cnt;              /* 下面这个数组里的元素的数量 */
    struct vm_area_struct vmas[0]; /* 可以开大小为 0 的数组，这个定义不可以和前面的 vma_cnt 换个位置*/
};

struct pt_regs {
    // 31个通用寄存器 from x0 to x31
    uint64 ra;
    uint64 sp;
    uint64 gp;
    uint64 tp;
    uint64 t0;
    uint64 t1;
    uint64 t2;
    uint64 s0; // or fp
    uint64 s1;
    uint64 a0;
    uint64 a1;
    uint64 a2;
    uint64 a3;
    uint64 a4;
    uint64 a5;
    uint64 a6;
    uint64 a7;
    uint64 s2;
    uint64 s3;
    uint64 s4;
    uint64 s5;
    uint64 s6;
    uint64 s7;
    uint64 s8;
    uint64 s9;
    uint64 s10;
    uint64 s11;
    uint64 t3;
    uint64 t4;
    uint64 t5;
    uint64 t6;

    uint64 sepc;
    uint64 sstatus;
    uint64 sscratch;
    uint64 stval;
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

// 创建一个新的 vma
void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
             uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file);

// 查找包含某个 addr 的 vma，该函数主要在 Page Fault 处理时起作用。
struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr);