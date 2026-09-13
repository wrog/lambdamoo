#include "arena.h"

#include "config.h"
#include "options.h"

#define ALIGN_UP(size, align) (((size) + (align) - 1) & ~((align) - 1))
#define ARENA_ALIGNMENT (sizeof(void *))

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t capacity;
    size_t used;
    char data[];
} ArenaBlock;

struct Arena {
    ArenaBlock *head;
    ArenaBlock *current;
    size_t default_block_size;
    Memory_Type type;
};

static ArenaBlock *
allocate_block(size_t capacity, Memory_Type type)
{
    ArenaBlock *block = mymalloc(sizeof(ArenaBlock) + capacity, type);

    block->next = NULL;
    block->capacity = capacity;
    block->used = 0;

    return block;
}

Arena *
arena_create(size_t default_block_size, Memory_Type type)
{
    Arena *arena = mymalloc(sizeof(Arena), type);

    arena->type = type;
    arena->default_block_size = default_block_size;
    arena->head = allocate_block(default_block_size, type);
    arena->current = arena->head;

    return arena;
}

void *
arena_alloc(Arena *arena, size_t size)
{
    size_t aligned_size = ALIGN_UP(size, ARENA_ALIGNMENT);

    if (arena->current->capacity - arena->current->used < aligned_size) {
	size_t new_capacity =
	    (aligned_size > arena->default_block_size) ? aligned_size
	    : arena->default_block_size;
	ArenaBlock *new_block = allocate_block(new_capacity, arena->type);

	arena->current->next = new_block;
	arena->current = new_block;
    }

    void *ptr = arena->current->data + arena->current->used;

    arena->current->used += aligned_size;
    return ptr;
}

void
arena_destroy(Arena *arena)
{
    ArenaBlock *current = arena->head;
    Memory_Type type = arena->type;

    while (current) {
	ArenaBlock *next = current->next;

	myfree(current, type);
	current = next;
    }

    myfree(arena, type);
}
