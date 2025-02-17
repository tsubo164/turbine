#ifndef RUNTIME_QUEUE_H
#define RUNTIME_QUEUE_H

#include "runtime_gc.h"
#include "runtime_value.h"
#include <stdint.h>

struct runtime_queue {
    struct runtime_object obj;
    struct runtime_valuevec values;
    int val_type;
    int front;
    int back;

    compare_function_t compare;
};

struct runtime_queue *runtime_queue_new(int val_type, int64_t len);
void runtime_queue_free(struct runtime_queue *s);

/*
int64_t runtime_queue_len(const struct runtime_queue *s);
bool runtime_queue_empty(const struct runtime_queue *s);
void runtime_queue_push(struct runtime_queue *s, struct runtime_value val);
struct runtime_value runtime_queue_pop(struct runtime_queue *s);
struct runtime_value runtime_queue_top(const struct runtime_queue *s);
*/

/* No index range check */
//struct runtime_value runtime_queue_get(const struct runtime_queue *s, int64_t idx);

#endif /* _H */
