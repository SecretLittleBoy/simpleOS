#include "proc.h"
#include "mm.h"
#include "defs.h"
#include "rand.h"
#include "printk.h"
#include "test.h"
#include "elf.h"
extern void __dummy();

struct task_struct *idle;           // idle process
struct task_struct *current;        // 指向当前运行线程的 `task_struct`
struct task_struct *task[NR_TASKS]; // 线程数组, 所有的线程都保存在此

extern uint64 task_test_priority[]; // test_init 后, 用于初始化 task[i].priority 的数组
extern uint64 task_test_counter[];  // test_init 后, 用于初始化 task[i].counter  的数组
extern uint64 swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));
extern char _sramdisk[];
extern char _eramdisk[];

static uint64_t load_program(struct task_struct *task) {
    // ELF简要布局：https://zhuanlan.zhihu.com/p/286088470
    // ELF64_Phdr详解：https://zhuanlan.zhihu.com/p/389408697
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)_sramdisk;           // 此时指向elf数据头
    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff; // 指向数据体
    int phdr_cnt = ehdr->e_phnum;                         // segement的metedata数量

    Elf64_Phdr *phdr;
    for (int i = 0; i < phdr_cnt; i++) {                            // 遍历每一个segement
        phdr = (Elf64_Phdr *)(phdr_start + sizeof(Elf64_Phdr) * i); // 获取当前segement的数据指针
        if (phdr->p_type == PT_LOAD) {
            uint64 vaddr_round = (uint64)(phdr->p_vaddr) - PGROUNDDOWN(phdr->p_vaddr);

            uint64 num_pages_to_copy = (vaddr_round + phdr->p_memsz) / PGSIZE + 1;
            uint64 pages_dest_addr = alloc_pages(num_pages_to_copy);
            uint64 pages_src_addr = (uint64)(_sramdisk) + phdr->p_offset; // p_offset：段内容的开始位置相对于文件开头的偏移量
            memcpy((uint64 *)(pages_dest_addr + vaddr_round), (uint64 *)pages_src_addr, phdr->p_memsz);

            uint64 perms = phdr->p_flags;
            uint64 perm_r = (perms & 4) >> 1, perms_w = (perms & 2) << 1, perm_x = (perms & 1) << 3;
            uint64 pages_perms = PTE_USER | perm_x | perms_w | perm_r | PTE_VALID;
            // p_flags: 2|1|0   page table entry: 4|3|2|1|0
            //          R|W|X                     U|X|W|R|V

            create_mapping((uint64 *)task->pgd, (uint64)PGROUNDDOWN(phdr->p_vaddr),
                           pages_dest_addr - PA2VA_OFFSET, num_pages_to_copy * PGSIZE, pages_perms);
        }
    }

    // allocate user stack and do mapping
    uint64 addr = alloc_page();
    create_mapping(task->pgd, (uint64)(USER_END)-PGSIZE, (uint64)(addr - PA2VA_OFFSET), PGSIZE, PTE_USER | PTE_WRITE | PTE_READ | PTE_VALID); // 映射用户栈 U-WRV

    task->thread.sepc = ehdr->e_entry;
    task->thread.sstatus = SSTATUS_SPP_UMODE | SSTATUS_SPIE | SSTATUS_SUM;
    task->thread.sscratch = USER_END;
}

static uint64_t load_program_without_create_mapping(struct task_struct *task) {
    // ELF简要布局：https://zhuanlan.zhihu.com/p/286088470
    // ELF64_Phdr详解：https://zhuanlan.zhihu.com/p/389408697
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)_sramdisk;           // 此时指向elf数据头
    uint64_t phdr_start = (uint64_t)ehdr + ehdr->e_phoff; // 指向数据体
    int phdr_cnt = ehdr->e_phnum;                         // segement的metedata数量

    Elf64_Phdr *phdr;
    for (int i = 0; i < phdr_cnt; i++) {                            // 遍历每一个segement
        phdr = (Elf64_Phdr *)(phdr_start + sizeof(Elf64_Phdr) * i); // 获取当前segement的数据指针
        if (phdr->p_type == PT_LOAD) {
            uint64 vaddr_round = (uint64)(phdr->p_vaddr) - PGROUNDDOWN(phdr->p_vaddr);

            uint64 num_pages_to_copy = (vaddr_round + phdr->p_memsz) / PGSIZE + 1;
            uint64 pages_dest_addr = alloc_pages(num_pages_to_copy);
            uint64 pages_src_addr = (uint64)(_sramdisk) + phdr->p_offset; // p_offset：段内容的开始位置相对于文件开头的偏移量
            memcpy((uint64 *)(pages_dest_addr + vaddr_round), (uint64 *)pages_src_addr, phdr->p_memsz);

            uint64 perms = phdr->p_flags;
            uint64 perm_r = (perms & 4) >> 1, perms_w = (perms & 2) << 1, perm_x = (perms & 1) << 3;
            uint64 pages_perms = PTE_USER | perm_x | perms_w | perm_r | PTE_VALID;
            // p_flags: 2|1|0   page table entry: 4|3|2|1|0
            //          R|W|X                     U|X|W|R|V

            create_mapping((uint64 *)task->pgd, (uint64)PGROUNDDOWN(phdr->p_vaddr),
                           pages_dest_addr - PA2VA_OFFSET, num_pages_to_copy * PGSIZE, pages_perms);
        }
    }

    // allocate user stack and do mapping
    uint64 addr = alloc_page();
    create_mapping(task->pgd, (uint64)(USER_END)-PGSIZE, (uint64)(addr - PA2VA_OFFSET), PGSIZE, PTE_USER | PTE_WRITE | PTE_READ | PTE_VALID); // 映射用户栈 U-WRV

    task->thread.sepc = ehdr->e_entry;
    task->thread.sstatus = SSTATUS_SPP_UMODE | SSTATUS_SPIE | SSTATUS_SUM;
    task->thread.sscratch = USER_END;
}

static void useBinFile(struct task_struct *task) {
    task->thread.sepc = USER_START;                                        // 将 sepc 设置为 USER_START
    task->thread.sstatus = SSTATUS_SPP_UMODE | SSTATUS_SPIE | SSTATUS_SUM; // 配置 sstatus 中的 SPP（使得 sret 返回至 U-Mode）， SPIE （sret 之后开启中断）， SUM（S-Mode 可以访问 User 页面）
    task->thread.sscratch = USER_END;                                      // 将 sscratch 设置为 U-Mode 的 sp，其值为 USER_END （即，用户态栈被放置在 user space 的最后一个页面）。

    // 将 uapp 所在的页面映射到每个进程的页表中。
    // 注意，在程序运行过程中，有部分数据不在栈上，而在初始化的过程中就已经被分配了空间
    // 所以，二进制文件需要先被拷贝到一块某个进程专用的内存之后
    // 再进行映射，防止所有的进程共享数据，造成预期外的进程间相互影响

    /* 将二进制文件需要拷贝到一块某个进程专用的内存 */
    uint64 num_pages_to_copy = (_eramdisk - _sramdisk) / PGSIZE + 1;
    uint64 pages_dest_addr = alloc_pages(num_pages_to_copy);
    uint64 pages_src_addr = (uint64)_sramdisk;
    // p_offset：段内容的开始位置相对于文件开头的偏移量
    memcpy((uint64 *)pages_dest_addr, (uint64 *)pages_src_addr, num_pages_to_copy * PGSIZE);

    create_mapping((uint64 *)task->pgd, (uint64)USER_START,
                   pages_dest_addr - PA2VA_OFFSET, num_pages_to_copy * PGSIZE, PTE_USER | PTE_EXECUTE | PTE_WRITE | PTE_READ | PTE_VALID);
    // allocate user stack and do mapping
    uint64 user_stack_addr = alloc_page();
    create_mapping((uint64 *)task->pgd, USER_END - PGSIZE, user_stack_addr - PA2VA_OFFSET, PGSIZE, PTE_USER | PTE_WRITE | PTE_READ | PTE_VALID);
}

void task_init() {
    test_init(NR_TASKS);
    printk("task_init start\n");
    // 1. 调用 alloc_page() 为 idle 分配一个物理页
    // 2. 设置 state 为 TASK_RUNNING;
    // 3. 由于 idle 不参与调度 可以将其 counter / priority 设置为 0
    // 4. 设置 idle 的 pid 为 0
    // 5. 将 current 和 task[0] 指向 idle
    idle = (struct task_struct *)alloc_page();
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
    // 4. 其中 `ra` 设置为 __dummy 的地址,  `sp` 设置为 该线程申请的物理页的高地址
    for (int i = 1; i < NR_TASKS; ++i) {
        task[i] = (struct task_struct *)alloc_page();
        task[i]->state = TASK_RUNNING;
        task[i]->counter = task_test_counter[i];
        task[i]->priority = task_test_priority[i];
        task[i]->pid = i;
        task[i]->thread.ra = (uint64)__dummy;
        task[i]->thread.sp = (uint64)task[i] + PGSIZE - 1;

        task[i]->pgd = alloc_page();
        memcpy(task[i]->pgd, swapper_pg_dir, PGSIZE); // 为了避免 U-Mode 和 S-Mode 切换的时候切换页表，我们将内核页表 （ swapper_pg_dir ） 复制到每个进程的页表中。

        load_program(task[i]);
        //load_program_without_create_mapping(task[i]);
        // useBinFile(task[i]);
    }
    printk("...task_init done!\n");
}

void dummy() { // 一个内核态程序
    schedule_test();
    uint64 MOD = 1000000007;
    uint64 auto_inc_local_var = 0;
    int last_counter = -1;
    while (1) {
        if ((last_counter == -1 || current->counter != last_counter) && current->counter > 0) {
            last_counter = current->counter;
            auto_inc_local_var = (auto_inc_local_var + 1) % MOD;
            printk("[PID = %d] is running. auto_inc_local_var = %d,current->counter= %d,address= %x\n",
                   current->pid, auto_inc_local_var, current->counter, (uint64)current);
            if (current->counter == 1) { // forced the counter to be zero if this thread is going to be scheduled
                --(current->counter);    // in case that the new counter is also 1, leading the information not printed.
            }
        }
    }
}

// 创建一个新的 vma
void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
             uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file){

             }

// 查找包含某个 addr 的 vma，该函数主要在 Page Fault 处理时起作用。
struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr);

void do_timer() {
    // 1. 如果当前线程是 idle 线程 直接进行调度
    // 2. 如果当前线程不是 idle 对当前线程的运行剩余时间减1 若剩余时间仍然大于0 则直接返回，否则进行调度
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
        printk("task[%d]->priority = %d, counter = %d\n", i, task[i]->priority, task[i]->counter);
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
    printk("switch_to ing  maxIndex = %d\n", maxIndex);
    switch_to(task[maxIndex]);
#endif
}

extern void __switch_to(struct task_struct *prev, struct task_struct *next, uint64 next_phy_pgtbl);
void switch_to(struct task_struct *next) {
    if (current != next) {
        struct task_struct *prev = current;
        current = next;
        // get next's satp Register
        uint64 next_phy_pgtbl = PhysicalAddress2PPN(VA2PA(next->pgd)) | Sv39Mode;
        __switch_to(&(prev->thread), &(next->thread), next_phy_pgtbl);
    }
}
