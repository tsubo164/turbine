#include "code_constant_pool.h"
#include "gc.h"

void code_constant_pool_init(struct code_constant_pool *v)
{
    ValueVecInit(&v->ints);
    ValueVecInit(&v->floats);
    ValueVecInit(&v->strings);
}

void code_constant_pool_free(struct code_constant_pool *v)
{
    ValueVecFree(&v->ints);
    ValueVecFree(&v->floats);
    ValueVecFree(&v->strings);
}

int code_constant_pool_push_int(struct code_constant_pool *v, value_int_t val)
{
    for (int i = 0; i < v->ints.len; i++) {
        if (v->ints.data[i].inum == val)
            return i;
    }

    struct Value value;
    value.inum = val;
    ValueVecPush(&v->ints, value);

    int new_idx = v->ints.len - 1;
    return new_idx;
}

int code_constant_pool_push_float(struct code_constant_pool *v, value_float_t val)
{
    for (int i = 0; i < v->floats.len; i++) {
        if (v->floats.data[i].fpnum == val)
            return i;
    }

    struct Value value;
    value.fpnum = val;
    ValueVecPush(&v->floats, value);

    int new_idx = v->floats.len - 1;
    return new_idx;
}

int code_constant_pool_push_string(struct code_constant_pool *v, const char *val)
{
    for (int i = 0; i < v->strings.len; i++) {
        struct Value s = ValueVecGet(&v->strings, i);
        if (runtime_string_compare_cstr(s.str, val) == 0)
            return i;
    }

    struct Value value;
    value.str = GCStringNew(val);
    ValueVecPush(&v->strings, value);

    int new_idx = v->strings.len - 1;
    return new_idx;
}

struct Value code_constant_pool_get_int(const struct code_constant_pool *v, int id)
{
    return ValueVecGet(&v->ints, id);
}

struct Value code_constant_pool_get_float(const struct code_constant_pool *v, int id)
{
    return ValueVecGet(&v->floats, id);
}

struct Value code_constant_pool_get_string(const struct code_constant_pool *v, int id)
{
    return ValueVecGet(&v->strings, id);
}
