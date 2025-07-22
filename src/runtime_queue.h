#ifndef RUNTIME_QUEUE_H
#define RUNTIME_QUEUE_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdbool.h>

struct runtime_queue {
    struct runtime_object obj;
    struct runtime_value *data;
    int val_type;
    value_int_t len;
    value_int_t cap;
    value_int_t front;
    value_int_t back;

    compare_function_t compare;
};

struct runtime_queue *runtime_queue_new(struct runtime_gc *gc, int val_type, value_int_t len);
void runtime_queue_free(struct runtime_gc *gc, struct runtime_queue *q);

value_int_t runtime_queue_len(const struct runtime_queue *q);
bool runtime_queue_empty(const struct runtime_queue *q);
struct runtime_value runtime_queue_front(const struct runtime_queue *q);

/* gc managed */
void runtime_queue_push(struct runtime_gc *gc, struct runtime_queue *q, struct runtime_value val);
struct runtime_value runtime_queue_pop(struct runtime_gc *gc, struct runtime_queue *q);

/* no index range check */
struct runtime_value runtime_queue_get(const struct runtime_queue *q, value_int_t idx);

#endif /* _H */
