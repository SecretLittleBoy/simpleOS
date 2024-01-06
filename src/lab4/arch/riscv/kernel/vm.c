// arch/riscv/kernel/vm.c
#include "types.h"
#include "defs.h"
#include "string.h"
#include "printk.h"

/* early_pgtbl: 用于 setup_vm 进行 1GB 的 映射。 */
unsigned long early_pgtbl[512] __attribute__((__aligned__(0x1000)));

void setup_vm(void) {
    /*
    1. 由于是进行 1GB 的映射 这里不需要使用多级页表
    2. 将 va 的 64bit 作为如下划分： | high bit | 9 bit | 30 bit |
        high bit 可以忽略
        中间9 bit 作为 early_pgtbl 的 index
        低 30 bit 作为 页内偏移 这里注意到 30 = 9 + 9 + 12， 即我们只使用根页表， 根页表的每个 entry 都对应 1GB 的区域。
    3. Page Table Entry 的权限 V | R | W | X 位设置为 1
    */
    /*
    Physical Address
    -------------------------------------------
                         | OpenSBI | Kernel |
    -------------------------------------------
                         ↑
                    0x80000000
                         ├───────────────────────────────────────────────────┐
                         |                                                   |
    Virtual Address      ↓                                                   ↓
    -----------------------------------------------------------------------------------------------
                         | OpenSBI | Kernel |                                | OpenSBI | Kernel |
    -----------------------------------------------------------------------------------------------
                         ↑                                                   ↑
                    0x80000000                                       0xffffffe000000000
    */
    printk("setup_vm\n");
    // 0x20000000U == 0x80000 << 10，PPN字段从PTE的第10位开始，因此左移10位
    //  early_pgtbl[L2] = (uint64)(0 | 0x80000000>>30 | 15U)
    //  0x80000000>>30,再取低9位即为L32
    early_pgtbl[2] = (uint64)(0 | 0x20000000U | 15U); // 倒数四位为1111
    // early_pgtbl[L2] = (uint64)(0 | 0x80000000>>30 | 15U)
    // 0xffffffe000000000>>30,再取低9位即为L2=0x180 , 即Sv39 virtual address的VPN[2]
    early_pgtbl[0x180] = (uint64)(0 | 0x20000000U | 15U); // 倒数四位为1111
}

extern uint64 _stext;
extern uint64 _srodata;
extern uint64 _sdata;

/* swapper_pg_dir: kernel pagetable 根目录， 在 setup_vm_final 进行映射。 */
unsigned long swapper_pg_dir[512] __attribute__((__aligned__(0x1000)));

void setup_vm_final(void) {
    printk("setup_vm_final\n");
    memset(swapper_pg_dir, 0x0, PGSIZE);

    // No OpenSBI mapping required

    // mapping kernel text X|-|R|V
    create_mapping((uint64 *)swapper_pg_dir, (uint64)&_stext, (uint64)&_stext - PA2VA_OFFSET,
                   (uint64)&_srodata - (uint64)&_stext, 0B1011);

    // mapping kernel rodata -|-|R|V
    create_mapping((uint64 *)swapper_pg_dir, (uint64)&_srodata, (uint64)&_srodata - PA2VA_OFFSET,
                   (uint64)&_sdata - (uint64)&_srodata, 0B0011);

    // mapping other memory -|W|R|V
    create_mapping((uint64 *)swapper_pg_dir, (uint64)&_sdata, (uint64)&_sdata - PA2VA_OFFSET,
                   PHY_END + PA2VA_OFFSET - (uint64)&_sdata, 0B0111);
    printk("create_mapping all done\n");

    printk("satp(old): %llx\n", csr_read(satp));
    uint64 satp_ = (8UL << 60) | (((uint64)swapper_pg_dir - PA2VA_OFFSET) >> 12);
    printk("satp(target): %llx\n", satp_);
    csr_write(satp, satp_);

    printk("satp(new): %llx\n", csr_read(satp));
    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    printk("setup_vm_final done\n");
    return;
}

/**** 创建多级页表映射关系 *****/
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, uint64 perm) {
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小，单位为字节
    perm 为映射的权限 (即页表项的低 8 位)

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */
    while (sz > 0) {
        uint64 vpn2 = (va >> 30) & 0x1ff;
        uint64 vpn1 = (va >> 21) & 0x1ff;
        uint64 vpn0 = (va >> 12) & 0x1ff;

        // 第二级页表
        uint64 *pgtbl1;
        // 检查有效
        if (!(pgtbl[vpn2] & 1)) {
            pgtbl1 = (uint64 *)kalloc();
            pgtbl[vpn2] = (1 | (((uint64)pgtbl1 - PA2VA_OFFSET) >> 12 << 10));
        } else {
            pgtbl1 = (uint64 *)((pgtbl[vpn2] >> 10 << 12) + PA2VA_OFFSET);
        }

        // 第三级页表
        uint64 *pgtbl0;
        // 检查有效
        if (!(pgtbl1[vpn1] & 1)) {
            pgtbl0 = (uint64 *)kalloc();
            pgtbl1[vpn1] = (1 | (((uint64)pgtbl0 - PA2VA_OFFSET) >> 12 << 10));
        } else {
            pgtbl0 = (uint64 *)((pgtbl1[vpn1] >> 10 << 12) + PA2VA_OFFSET);
        }

        // 物理页
        pgtbl0[vpn0] = (perm | (pa >> 12 << 10));

        // 映射下一页
        sz -= 0x1000;
        va += 0x1000;
        pa += 0x1000;
    }
}