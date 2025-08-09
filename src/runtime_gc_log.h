#ifndef RUNTIME_GC_LOG_H
#define RUNTIME_GC_LOG_H

#include "value_types.h"
#include <stdlib.h>

struct runtime_gc_log_entry {
    value_addr_t triggered_addr;
    size_t used_bytes_before;
    size_t used_bytes_after;
    int total_collections;
};

struct runtime_gc_log {
    struct runtime_gc_log_entry *data;
    int front;
    int back;
    int len;
};

void runtime_gc_log_init_entry(struct runtime_gc_log_entry *ent);

void runtime_gc_log_init(struct runtime_gc_log *log);
void runtime_gc_log_clear(struct runtime_gc_log *log);

int runtime_gc_log_get_count(const struct runtime_gc_log *log);
const struct runtime_gc_log_entry *runtime_gc_log_get_entry(const struct runtime_gc_log *log, int index);
void runtime_gc_log_push(struct runtime_gc_log *log, const struct runtime_gc_log_entry *ent);

#endif /* _H */
