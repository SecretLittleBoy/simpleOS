#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"

extern void __dummy();

struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 `task_struct`
struct task_struct *task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

/**
 * new content for unit test of 2023 OS lab2
 */
extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组

void task_init() {
    test_init(NR_TASKS);
    // 1. 调用 kalloc() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    uint64 idelAddr = kalloc();
    idle = (struct task_struct *)idelAddr;
    idle->state = TASK_RUNNING;
    idle->counter = 0;
    idle->priority = 0;
    idle->pid = 0;
    current = idle;
    task[0] = idle;

    // 1. 参考 idle 的设置, 为 task[1] ~ task[NR_TASKS - 1] 进行初始化
    // 2. 其中每个线程的 state 为 TASK_RUNNING, 此外，为了单元测试的需要，counter 和 priority 进行如下赋值：
    //      task[i].counter  = task_test_counter[i];
    //      task[i].priority = task_test_priority[i];
    // 3. 为 task[1] ~ task[NR_TASKS - 1] 设置 `thread_struct` 中的 `ra` 和 `sp`,
    // 4. 其中 `ra` 设置为 __dummy （见 4.3.2）的地址,  `sp` 设置为 该线程申请的物理页的高地址
    for (int i = 1; i < NR_TASKS; ++i) {
        uint64 addr = kalloc();
        task[i] = (struct task_struct *)addr;
        task[i]->state = TASK_RUNNING;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = addr + PGSIZE;
    }

    printk("...proc_init done!\n");
}

void dummy() {
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while (1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d,current->counter= %d\n",
                   current->pid, auto_inc_local_var, current->counter);
            if (current->counter == 1) { // forced the counter to be zero if this thread is going to be scheduled
                --(current->counter);    // in case that the new counter is also 1, leading the information not printed.
            }
        }
    }
}

void do_timer() {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回 否则进行调度
    if (current == idle) {
        printk("current is idle,going to schedule\n");
        schedule();
    } else {
        if (current->counter > 0) {
            --(current->counter);
            return;
        } else {
            schedule();
        }
    }
}

void schedule() {
#ifdef SJF
    printk("start SHORT_JOB_FIRST_SCHEDULE\n");
START_OF_SHORT_JOB_FIRST_SCHEDULE:
    int min = 0x3f3f3f3f;
    int minIndex = -1;
    for (int i = 1; i < NR_TASKS; ++i) {
        printk("task[%d]->counter = %d\n", i, task[i]->counter);
        if (task[i]->counter < min && task[i]->counter > 0) {
            min = task[i]->counter;
            minIndex = i;
        }
    }
    if (minIndex == -1) {
        for (int i = 1; i < NR_TASKS; ++i) {
            task[i]->counter = rand() % 10 + 1;
        }
        goto START_OF_SHORT_JOB_FIRST_SCHEDULE;
    }
    printk("switch_to ing  minIndex = %d\n", minIndex);
    switch_to(task[minIndex]);

#elif defined(PRIORITY_SCHEDULE)
    printk("start PRIORITY_SCHEDULE\n");
START_OF_PRIORITY_SCHEDULE:
    uint64 max = 0;
    int maxIndex = 0;
    for (int i = 1; i < NR_TASKS; ++i) {
        //printk("task[%d]->priority = %d, counter = %d\n", i, task[i]->priority, task[i]->counter);
        if (task[i]->priority > max && task[i]->counter > 0) {
            max = task[i]->priority;
            maxIndex = i;
        }
    }
    if (maxIndex == 0) {
        for (int i = 1; i < NR_TASKS; ++i) {
            task[i]->counter = rand() % 10 + 1;
        }
        goto START_OF_PRIORITY_SCHEDULE;
    }
    //printk("switch_to ing  maxIndex = %d\n", maxIndex);
    switch_to(task[maxIndex]);
#endif
}

extern void __switch_to(struct task_struct *prev, struct task_struct *next);
void switch_to(struct task_struct *next) {
    if (current != next) {
        struct task_struct *prev = current;
        current = next;
        __switch_to(prev, next);
    }
}