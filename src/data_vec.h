#ifndef DATA_VEC_H
#define DATA_VEC_H

#include <stdbool.h>
#include <stdint.h>

/* Vector of int */
struct data_intvec {
    int64_t *data;
    int cap;
    int len;
};

void data_intvec_init(struct data_intvec *v);
void data_intvec_resize(struct data_intvec *v, int new_len);
void data_intvec_push(struct data_intvec *v, int64_t val);
void data_intvec_free(struct data_intvec *v);

/* Stack of int */
struct data_intstack {
    int64_t *data;
    int cap;
    int len;
};

void data_intstack_push(struct data_intstack *v, int64_t val);
/* Does not check empty */
int64_t data_intstack_pop(struct data_intstack *v);
int64_t data_intstack_top(const struct data_intstack *v);
bool data_intstack_is_empty(const struct data_intstack *v);

#endif /* _H */
