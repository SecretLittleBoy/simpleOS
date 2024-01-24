#include "syscall.h"
#include "proc.h"
#include "stddef.h"
#include "printk.h"
#include "defs.h"
#include "string.h"
extern struct task_struct *current;
extern struct task_struct *task[NR_TASKS];
extern void __ret_from_fork();
extern unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

// 返回打印的字符数
long sys_write(unsigned int fd, const char *buf, size_t count) {
    if (fd == 1) {
        size_t i;
        for (i = 0; i < count; i++) {
            if (buf[i] == '\0')
                break;
            putc(buf[i]);
        }
        return i;
    } else {
        return 0;
    }
}

long sys_getpid() {
    return current->pid;
}

unsigned long sys_clone(struct pt_regs *regs) {
    /*
     1. 参考 task_init 创建一个新的 task，将的 parent task 的整个页复制到新创建的
        task_struct 页上(这一步复制了哪些东西?）。将 thread.ra 设置为
        __ret_from_fork，并正确设置 thread.sp
        (仔细想想，这个应该设置成什么值?可以根据 child task 的返回路径来倒推)*/
    int i;
    for (i = 1; i < NR_TASKS; i++) {
        if (task[i] == NULL) {
            break;
        }
    }
    if (i == NR_TASKS) {
        printk("task is full\n");
        return -1;
    }
    struct task_struct *task_child = (struct task_struct *)alloc_page();
    memcpy(task_child, current, PGSIZE);
    task_child->pid = i;
    task[i] = task_child;
    task_child->thread.ra = (uint64)__ret_from_fork;

    /*
    2. 利用参数 regs 来计算出 child task 的对应的 pt_regs 的地址，
    并将其中的 a0, sp, sepc 设置成正确的值(为什么还要设置 sp?)*/
    uint64 ofs_pt_regs = (uint64)regs - (uint64)current;
    struct pt_regs *pt_regs_child = (struct pt_regs *)((uint64)task_child + ofs_pt_regs);
    memcpy(pt_regs_child, regs, sizeof(struct pt_regs));
    task_child->thread.sp = task_child + (uint64)regs - (uint64)current;
    pt_regs_child->a0 = 0;                     // a0（子进程返回值）
    pt_regs_child->sp = (uint64)pt_regs_child; // sp
    pt_regs_child->sepc = regs->sepc + 4;      // sepc（调用fork的下一条）
    task_child->thread.sp = (uint64)pt_regs_child; // 需要通过sp来恢复所有寄存器的值（__ret_from_trap）

    /*
     3. 为 child task 分配一个根页表，并仿照 setup_vm_final 来创建内核空间的映射
    */
    task_child->pgd = (pagetable_t)alloc_page();
    memcpy(task_child->pgd, swapper_pg_dir, PGSIZE);

    /*
    4. 为 child task 申请 user stack，并将 parent task 的 user stack
       数据复制到其中。 (既然 user stack 也在 vma 中，这一步也可以直接在 5 中做，无需特殊处理)*/

    uint64 task_child_user_stack = alloc_page();
    memcpy((char *)task_child_user_stack, (char *)(USER_END - PGSIZE), PGSIZE);
    create_mapping(task_child->pgd, USER_END - PGSIZE, task_child_user_stack - PA2VA_OFFSET, PGSIZE,
                   PTE_USER | PTE_WRITE | PTE_READ | PTE_VALID);

    /*
     5. 根据 parent task 的页表和 vma 来分配并拷贝 child task 在用户态会用到的内存
    */
    for (i = 0; i < current->vma_cnt; i++) {
        struct vm_area_struct vma = current->vmas[i];
        uint64 vm_addr_curr = vma.vm_start;
        while (vm_addr_curr < vma.vm_end) {
            uint64 vm_addr_pg = PGROUNDDOWN(vm_addr_curr);
            /* 映射存在 */
            if (get_mapping(current->pgd, vm_addr_curr)) {
                uint64 pg_copy = alloc_page();
                memcpy((char *)pg_copy, (char *)vm_addr_pg, PGSIZE);
                create_mapping(task_child->pgd, vm_addr_pg, pg_copy - PA2VA_OFFSET,
                               PGSIZE, vma.vm_flags | PTE_USER | PTE_VALID);
            }
            vm_addr_curr = vm_addr_pg + PGSIZE;
        }
    }

    /*
     6. 返回子 task 的 pid
    */
    return task_child->pid;
}
