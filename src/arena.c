#include "arena.h"
#include "error.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct ucd_arena {
    struct ucd_arena *prev;
    size_t cap;
    size_t used;
    char data[];
} ucd_arena;

static _Thread_local ucd_arena *g_arena;

static const size_t ARENA_PAD =
    (alignof(max_align_t) -
     (offsetof(ucd_arena, data) % alignof(max_align_t))) %
    alignof(max_align_t);
static const size_t ARENA_HEAD = offsetof(ucd_arena, data) + ARENA_PAD;
static const size_t ARENA_MAX_CAP =
    SIZE_MAX - ARENA_HEAD - alignof(max_align_t) + 1;

static inline size_t calc_size(size_t cap)
{
    return ARENA_HEAD + cap;
}

static inline ucd_arena *init_arena(ucd_arena *arena, size_t cap)
{
    memset(arena, 0, ARENA_HEAD);
    arena->prev = g_arena;
    arena->cap = cap;
    return (g_arena = arena);
}

static inline ucd_arena *new_arena(size_t min, size_t cap)
{
    ucd_arena *arena;
    if ((arena = malloc(calc_size(cap)))) {
        return init_arena(arena, cap);
    }

    size_t slack = cap - min;
    do {
        slack /= 2;
        cap = min + slack;
        if ((arena = malloc(calc_size(cap)))) {
            return init_arena(arena, cap);
        }
    } while (slack);

    error_sys(UCD_ERROR_FATAL, errno, "Failed to allocate memory");
    return NULL;
}

static inline void *reserve_ptr(size_t align, size_t size)
{
    for (ucd_arena *arena = g_arena; arena; arena = arena->prev) {
        size_t pad = calc_align_pad(arena->used, align);
        size_t footprint = pad + size;
        if (footprint <= arena->cap - arena->used) {
            void *ptr = arena->data + arena->used + pad;
            arena->used += footprint;
            return ptr;
        }
    }
    return NULL;
}

void *arena_alloc(size_t align, size_t size)
{
    if (size > ARENA_MAX_CAP) {
        error_sys(UCD_ERROR_FATAL, EOVERFLOW, "Failed to allocate memory");
        return NULL;
    }

    void *ptr = reserve_ptr(align, size);
    if (ptr) {
        return ptr;
    } else if (!new_arena(
                   size + align - 1,
                   g_arena ? g_arena->cap : UCD_ARENA_INIT_CAP)) {
        return NULL;
    }
    return reserve_ptr(align, size);
}
