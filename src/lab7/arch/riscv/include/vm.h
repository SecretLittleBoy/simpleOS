#ifndef _VM_H_
#define _VM_H_

#include "proc.h"

void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm);

// 创建一个新的 vma
void do_mmap(struct task_struct *task, uint64_t addr, uint64_t length, uint64_t flags,
             uint64_t vm_content_offset_in_file, uint64_t vm_content_size_in_file);

// 查找包含某个 addr 的 vma，该函数主要在 Page Fault 处理时起作用。
struct vm_area_struct *find_vma(struct task_struct *task, uint64_t addr);

#endif