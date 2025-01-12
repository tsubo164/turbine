#ifndef RUNTIME_VALUE_H
#define RUNTIME_VALUE_H

#include <stdbool.h>
#include <stdint.h>

typedef int64_t  value_int_t;
typedef double   value_float_t;

struct runtime_map;
struct runtime_array;
struct runtime_string;
struct runtime_struct;

struct runtime_value {
    union {
        value_int_t inum;
        value_float_t fpnum;
        struct runtime_string *string;
        struct runtime_array *array;
        struct runtime_map *map;
        struct runtime_struct *strct;
    };
};

struct runtime_valuevec {
    struct runtime_value *data;
    int cap;
    int len;
};

void runtime_valuevec_init(struct runtime_valuevec *v);
bool runtime_valuevec_is_empty(const struct runtime_valuevec *v);
int runtime_valuevec_len(const struct runtime_valuevec *v);
void runtime_valuevec_resize(struct runtime_valuevec *v, int new_len);
void runtime_valuevec_push(struct runtime_valuevec *v, struct runtime_value val);
struct runtime_value runtime_valuevec_get(const struct runtime_valuevec *v, int idx);
void runtime_valuevec_set(struct runtime_valuevec *v, int idx, struct runtime_value val);
void runtime_valuevec_free(struct runtime_valuevec *v);

#endif /* _H */
