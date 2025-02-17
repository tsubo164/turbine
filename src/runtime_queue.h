#ifndef RUNTIME_QUEUE_H
#define RUNTIME_QUEUE_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_queue {
    struct runtime_object obj;
    struct runtime_value *data;
    int val_type;
    int len;
    int cap;
    int front;
    int back;

    compare_function_t compare;
};

struct runtime_queue *runtime_queue_new(int val_type, int64_t len);
void runtime_queue_free(struct runtime_queue *q);

int64_t runtime_queue_len(const struct runtime_queue *q);
bool runtime_queue_empty(const struct runtime_queue *q);
void runtime_queue_push(struct runtime_queue *q, struct runtime_value val);
/*
struct runtime_value runtime_queue_pop(struct runtime_queue *q);
struct runtime_value runtime_queue_top(const struct runtime_queue *q);
*/

/* No index range check */
//struct runtime_value runtime_queue_get(const struct runtime_queue *q, int64_t idx);

#endif /* _H */
