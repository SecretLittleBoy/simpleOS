#include "defs.h"
#include "string.h"
#include "mm.h"

#include "printk.h"

extern char _ekernel[];

struct {
    struct run *freelist; // freelist 用于跟踪空闲的内存块。
} kmem;

// The kalloc function removes a block from the freelist and returns its address.
uint64 kalloc() {
    struct run *r;

    r = kmem.freelist;
    kmem.freelist = r->next;

    memset((void *)r, 0x0, PGSIZE);
    return (uint64)r;
}

// The kfree function adds a block back to the freelist.
void kfree(uint64 addr) {
    struct run *r;

    // PGSIZE align
    addr = addr & ~(PGSIZE - 1);

    memset((void *)addr, 0x0, (uint64)PGSIZE);

    r = (struct run *)addr;
    r->next = kmem.freelist;
    kmem.freelist = r;

    return;
}

// The kfreerange function frees a range of memory.
void kfreerange(char *start, char *end) {
    char *addr = (char *)PGROUNDUP((uint64)start);
    for (; (uint64)(addr) + PGSIZE <= (uint64)end; addr += PGSIZE) {
        kfree((uint64)addr);
    }
}

// The mm_init function initializes the freelist.
void mm_init(void) {
    kfreerange(_ekernel, (char *)PHY_END);
    printk("...mm_init done!\n");
}
