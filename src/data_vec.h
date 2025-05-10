#ifndef DATA_VEC_H
#define DATA_VEC_H

#include "value_types.h"
#include <stdbool.h>

/* Vector of int */
struct data_intvec {
    value_int_t *data;
    int cap;
    int len;
};

void data_intvec_init(struct data_intvec *v);
void data_intvec_resize(struct data_intvec *v, int new_len);
void data_intvec_push(struct data_intvec *v, value_int_t val);
void data_intvec_free(struct data_intvec *v);

/* Stack of int */
struct data_intstack {
    value_int_t *data;
    int cap;
    int len;
};

void data_intstack_push(struct data_intstack *v, value_int_t val);
/* Does not check empty */
value_int_t data_intstack_pop(struct data_intstack *v);
value_int_t data_intstack_top(const struct data_intstack *v);
bool data_intstack_is_empty(const struct data_intstack *v);

void data_intstack_free(struct data_intstack *v);

#endif /* _H */
