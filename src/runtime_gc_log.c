#include "runtime_gc_log.h"
#include <assert.h>

void runtime_gc_log_init_entry(struct runtime_gc_log_entry *ent)
{
    struct runtime_gc_log_entry init = {0};
    *ent = init;
}

void runtime_gc_log_init(struct runtime_gc_log *log)
{
    log->data = calloc(128, sizeof(*log->data));
    log->front = 0;
    log->back = 0;
}

void runtime_gc_log_clear(struct runtime_gc_log *log)
{
    free(log->data);
    struct runtime_gc_log init = {0};
    *log = init;
}

int runtime_gc_log_get_count(const struct runtime_gc_log *log)
{
    return log->len;
}

const struct runtime_gc_log_entry *runtime_gc_log_get_entry(const struct runtime_gc_log *log, int index)
{
    assert(index >= 0 && index < log->len);

    int i = (log->front + index) % 128;
    return &log->data[i];
}

void runtime_gc_log_push(struct runtime_gc_log *log, const struct runtime_gc_log_entry *ent)
{
    log->data[log->back] = *ent;

    log->back = (log->back + 1) % 128;

    if (log->front == log->back)
        log->front = (log->front + 1) % 128;

    if (log->len < 128)
        log->len++;
}
