#ifndef RUNTIME_GC_WORKLIST_H
#define RUNTIME_GC_WORKLIST_H

#include "runtime_value.h"
#include <stdbool.h>

struct runtime_gc_worklist {
    struct runtime_value *data;
    value_int_t len;
    value_int_t cap;
    value_int_t front;
    value_int_t back;
};

void runtime_gc_worklist_init(struct runtime_gc_worklist *list);
void runtime_gc_worklist_clear(struct runtime_gc_worklist *list);

value_int_t runtime_gc_worklist_len(const struct runtime_gc_worklist *list);
bool runtime_gc_worklist_empty(const struct runtime_gc_worklist *list);
struct runtime_value runtime_gc_worklist_front(const struct runtime_gc_worklist *list);

void runtime_gc_worklist_push(struct runtime_gc_worklist *list, struct runtime_value val);
struct runtime_value runtime_gc_worklist_pop(struct runtime_gc_worklist *list);

#endif /* _H */
