#ifndef _DEFS_H
#define _DEFS_H

#include "types.h"

#define csr_read(csr)                 \
    ({                                \
        register uint64 __v;          \
        asm volatile("csrr %0 ," #csr \
                     : "=r"(__v)      \
                     :                \
                     : "memory");     \
        __v;                          \
    })

#define csr_write(csr, val)              \
    ({                                   \
        uint64 __v = (uint64)(val);      \
        asm volatile("csrw " #csr ", %0" \
                     : : "r"(__v)        \
                     : "memory");        \
    })

#define PHY_START 0x0000000080000000   // QEMU 环境中内存起始地址
#define PHY_SIZE 128 * 1024 * 1024     // QEMU 默认内存大小：128MB
#define PHY_END (PHY_START + PHY_SIZE) // 物理内存结束地址

#define PGSIZE 0x1000                                           // 页大小：4KB
#define PGROUNDUP(addr) ((addr + PGSIZE - 1) & (~(PGSIZE - 1))) // 4KB 对齐，向上取整
#define PGROUNDDOWN(addr) (addr & (~(PGSIZE - 1)))              // 4KB 对齐，向下取整

#define OPENSBI_SIZE (0x200000) // openSBI 会占用 0x80000000~0x80020000,所以head.S从 0x80020000 开始

/*Kernel 的虚拟内存布局
start_address             end_address
    0x0                  0x3fffffffff
     │                        │
┌────┘                  ┌─────┘
↓        256G           ↓
┌───────────────────────┬──────────┬────────────────┐
│      User Space       │    ...   │  Kernel Space  │
└───────────────────────┴──────────┴────────────────┘
                                   ↑      256G      ↑
                      ┌────────────┘                │
                      │                             │
              0xffffffc000000000           0xffffffffffffffff
                start_address                  end_address
*/
#define VM_START (0xffffffe000000000) // Kernel 的虚拟内存开始地址
#define VM_END (0xffffffff00000000)   // Kernel 的虚拟内存结束地址
#define VM_SIZE (VM_END - VM_START)   // Kernel 的虚拟内存大小

#define PA2VA_OFFSET (VM_START - PHY_START) // 0xffffffdf80000000

#define USER_START (0x0000000000000000) // user space start virtual address
#define USER_END (0x0000004000000000)   // user space end virtual address

#define SSTATUS_SPP_SMODE (1L << 8)
#define SSTATUS_SPP_UMODE (0L << 8)
#define SSTATUS_SPIE (1L << 5)
#define SSTATUS_SUM (1L << 18)

#define Sv39Mode (0x8L << 60)
#define PhysicalAddress2PPN(addr) ((uint64)(addr) >> 12)

// Page Table Entry DEFS
#define PTE_VALID 1
#define PTE_READ (1 << 1)
#define PTE_WRITE (1 << 2)
#define PTE_EXECUTE (1 << 3)
#define PTE_USER (1 << 4)

#endif
