#ifndef POOL_H
#define POOL_H

#include <stddef.h>

// See https://www.gingerbill.org/article/2019/02/16/memory-allocation-strategies-004/

#define POOL_SIZE 256

typedef struct Pool Pool;
typedef struct PoolFreeNode PoolFreeNode;

struct Pool {
    unsigned char *buf;
    size_t buf_len;
    size_t chunk_size;

    PoolFreeNode *head;
};

struct PoolFreeNode {
    PoolFreeNode *next;
};

void pool_init(
    Pool *p,
    void *backing_buffer,
    size_t backing_buffer_length,
    size_t chunk_size
    // size_t chunk_alignment
);
void *pool_alloc(Pool *p);
void pool_free(Pool *p, void *ptr);
void pool_free_all(Pool *p);

#define POOL_INIT(type, poolPtr) pool_init(poolPtr, malloc(sizeof(type) * POOL_SIZE), sizeof(type) * POOL_SIZE, sizeof(type))

#endif
