#pragma once

#include "types.h"

void *memset(void *, int, uint64);
void *memcpy(void *dst, void *src, uint64 len);

int memcmp(const void *cs, const void *ct, size_t count);

static inline int strlen(const char *str) {
    int len = 0;
    while (*str++)
        len++;
    return len;
}