#ifndef Arena_H
#define Arena_H 1

#include "storage.h"

typedef struct Arena Arena;

extern Arena *arena_create(size_t, Memory_Type);
extern void *arena_alloc(Arena *, size_t);
extern void arena_destroy(Arena *);

#endif		/* !Arena_H */
