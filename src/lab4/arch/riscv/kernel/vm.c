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
    create_mapping(swapper_pg_dir, (uint64)&_stext, (uint64)(&_stext) - PA2VA_OFFSET, 2, 11);

    // mapping kernel rodata -|-|R|V
    create_mapping(swapper_pg_dir, (uint64)&_srodata, (uint64)(&_srodata) - PA2VA_OFFSET, 1, 3);

    // mapping other memory -|W|R|V
    create_mapping(swapper_pg_dir, (uint64)&_sdata, (uint64)(&_sdata) - PA2VA_OFFSET, 32765, 7);
    printk("create_mapping all done\n");

    // asm volatile("csrw satp,%[src]" ::[src] "r"(satpValue) :);
    __asm__ volatile("csrw satp, %[src]" ::[src] "r"((uint64)swapper_pg_dir) :); // todo why??

    // flush TLB
    asm volatile("sfence.vma zero, zero");

    // flush icache
    asm volatile("fence.i");
    printk("setup_vm_final done\n");
    return;
}

/* 创建多级页表映射关系 */
/* 不要修改该接口的参数和返回值 */
void create_mapping(uint64 *pgtbl, uint64 va, uint64 pa, uint64 sz, int perm) { // sz的单位是4kb,即sz代表要映射的页面个数
    /*
    pgtbl 为根页表的基地址
    va, pa 为需要映射的虚拟地址、物理地址
    sz 为映射的大小，单位为字节
    perm 为映射的权限 (即页表项的低 8 位)

    创建多级页表的时候可以使用 kalloc() 来获取一页作为页表目录
    可以使用 V bit 来判断页表项是否存在
    */

    // 判断是leaf page table：pte.v == 1 ,并且pte.r或者pte.x其中一个为1
    while (sz--) {
        uint64 vpn2 = (va >> 30) & 0x1ff; // va中的30-39位为vpn2
        uint64 vpn1 = (va >> 21) & 0x1ff; // va中的21-29位为vpn1
        uint64 vpn0 = (va >> 12) & 0x1ff; // va中的12-20位为vpn0
        uint64 *pgtbl2;
        uint64 *pgtbl3;

        // 设置第二级页表
        if (pgtbl[vpn2] & 0x1) {
            pgtbl2 = ((pgtbl[vpn2] >> 10) & 0xfffffffffff) << 12; // 左移10位之后取44位，再右移12位
        } else {
            pgtbl2 = kalloc() - PA2VA_OFFSET; // 新申请一个页用于存储页表
            pgtbl[vpn2] = (((uint64)pgtbl2 >> 12) << 10); // 地址中的ppn位于12-55位，pte中的ppn位于10-53位
            pgtbl[vpn2] |= 0x1;
        }

        // 设置第三级页表
        if (pgtbl2[vpn1] & 0x1) {
            pgtbl3 = ((pgtbl2[vpn1] >> 10) & 0xfffffffffff) << 12;
        } else {
            pgtbl3 = kalloc() - PA2VA_OFFSET;
            pgtbl2[vpn1] = (uint64)pgtbl3 >> 2;
            pgtbl2[vpn1] |= 0x1;
        }

        // 第三级页表映射到物理页
        if (!(pgtbl3[vpn0] & 0x1)) {
            pgtbl3[vpn0] = (uint64)pa >> 2;
            pgtbl3[vpn0] &= 0xfffffffffffffff0; // perm 字段归零
            pgtbl3[vpn0] |= perm;
        }
        va += 0x1000, pa += 0x1000; // 0x1000 B= 4kB,即一个页面的大小
    }
}