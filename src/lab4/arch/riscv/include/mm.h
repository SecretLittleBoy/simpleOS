#include "types.h"

#define VA2PA(x) (((uint64)x - (uint64)PA2VA_OFFSET))
#define PA2VA(x) (((uint64)x + (uint64)PA2VA_OFFSET))

struct buddy {
    uint64 size;
    uint64 *bitmap;
};

uint64 alloc_pages(uint64);
uint64 alloc_page();
void free_pages(uint64);
void mm_init();