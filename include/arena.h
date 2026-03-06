#ifndef UCD_ARENA_H
#define UCD_ARENA_H

#include <stddef.h>

#define UCD_ARENA_INIT_CAP 128

static inline size_t mod_pow2(size_t x, size_t y)
{
    return x & (y - 1);
}

static inline size_t calc_align_pad(size_t offset, size_t align)
{
    return mod_pow2(align - mod_pow2(offset, align), align);
}

void *arena_alloc(size_t align, size_t size);

#endif
