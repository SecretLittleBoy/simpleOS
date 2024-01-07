#include "string.h"
#include "types.h"
void *memset(void *dst, int c, uint64 n) {
    char *cdst = (char *)dst;
    for (uint64 i = 0; i < n; ++i)
        cdst[i] = c;
    return dst;
}

void *memcpy(void *dst, void *src, uint64 len) {
    for (uint64 i = 0; i < len; ++i)
        ((char *)dst)[i] = ((char *)src)[i];

    return dst;
}

int memcmp(const void *cs, const void *ct, size_t count) {
    const unsigned char *su1, *su2;
    int res = 0;
    for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
        if ((res = *su1 - *su2) != 0)
            break;
    return res;
}