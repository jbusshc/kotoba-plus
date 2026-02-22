#ifndef ARENA_H
#define ARENA_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    uint8_t *base;
    size_t size;
    size_t offset;
} Arena;

static void arena_init(Arena *a, size_t size)
{
    a->base =  (uint8_t*)malloc(size);
    if (!a->base) {
        fprintf(stderr, "Arena allocation failed\n");
        exit(1);
    }
    a->size = size;
    a->offset = 0;
}

static void *arena_alloc(Arena *a, size_t size, size_t align)
{
    size_t current = (size_t)(a->base + a->offset);
    size_t aligned = (current + (align - 1)) & ~(align - 1);
    size_t new_offset = (aligned - (size_t)a->base) + size;

    if (new_offset > a->size) {
        fprintf(stderr, "Arena overflow\n");
        exit(1);
    }

    void *ptr = (void*)aligned;
    a->offset = new_offset;
    return ptr;
}

static void arena_reset(Arena *a)
{
    a->offset = 0;
}

static void arena_free(Arena *a)
{
    free(a->base);
}

#ifdef __cplusplus
}
#endif

#endif // ARENA_H
