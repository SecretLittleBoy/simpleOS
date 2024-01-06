#include "types.h"

#define VA2PA(x) ((x - (uint64)PA2VA_OFFSET))
#define PA2VA(x) ((x + (uint64)PA2VA_OFFSET))

struct run {
    struct run *next;
};

void mm_init();

uint64 kalloc();
void kfree(uint64);

struct buddy {
    uint64 size;
    uint64 *bitmap;
};

void buddy_init();
uint64 buddy_alloc(uint64);
void buddy_free(uint64);

uint64 alloc_pages(uint64);
uint64 alloc_page();
void free_pages(uint64);
