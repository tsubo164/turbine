#ifndef DATA_MEM_POOL_H
#define DATA_MEM_POOL_H

struct data_mem_block {
    void *data;
    int obj_size;
    int capacity;
    int used;
    struct data_mem_block *next;
};

struct data_mem_pool {
    int obj_size;
    int capacity;
    struct data_mem_block *blocks;
};

void data_mem_pool_init(struct data_mem_pool *pool, int obj_size, int capacity);
void data_mem_pool_clear(struct data_mem_pool *pool, void (*free_obj)(void *));
void *data_mem_pool_alloc(struct data_mem_pool *pool);
int data_mem_pool_alloc_count(const struct data_mem_pool *pool);

#endif /* _H */
