#include "data_mem_pool.h"

#include <stdlib.h>

void data_mem_pool_init(struct data_mem_pool *pool, int obj_size, int capacity)
{
    pool->obj_size = obj_size;
    pool->capacity = capacity;
    pool->blocks = NULL;
}

static struct data_mem_block *new_block(int obj_size, int capacity)
{
    struct data_mem_block *block;
    block = calloc(1, sizeof(*block));

    block->data = calloc(capacity, obj_size);
    if (!block->data) {
        free(block);
        return NULL;
    }

    block->obj_size = obj_size;
    block->capacity = capacity;
    block->used = 0;
    block->next = NULL;

    return block;
}

static void free_block(struct data_mem_block *block, void (*free_obj)(void *))
{
    if (free_obj) {
        for (int i = 0; i < block->used; i++) {
            void *obj = (char *) block->data + (i * block->obj_size);
            free_obj(obj);
        }
    }
    free(block->data);
    free(block);
}

static void *alloc_obj(struct data_mem_block *block)
{
    if (!block || block->used >= block->capacity)
        /* null or full */
        return NULL;

    void *obj = (char *) block->data + (block->used * block->obj_size);
    block->used++;

    return obj;
}

void *data_mem_pool_alloc(struct data_mem_pool *pool)
{
    void *obj = alloc_obj(pool->blocks);
    if (obj)
        return obj;

    /* need new block */
    struct data_mem_block *block;
    block = new_block(pool->obj_size, pool->capacity);

    /* add new block to the head */
    block->next = pool->blocks;
    pool->blocks = block;

    return alloc_obj(pool->blocks);
}

void data_mem_pool_clear(struct data_mem_pool *pool, void (*free_obj)(void *))
{
    struct data_mem_block *block = pool->blocks;

    while (block) {
        struct data_mem_block *next = block->next;
        free_block(block, free_obj);
        block = next;
    }
}

int data_mem_pool_alloc_count(const struct data_mem_pool *pool)
{
    struct data_mem_block *block;
    int count = 0;

    for (block = pool->blocks; block; block = block->next)
        count += block->used;

    return count;
}
