#include "code_constant_pool.h"
#include "runtime_string.h"

void code_constant_pool_init(struct code_constant_pool *v)
{
    runtime_valuevec_init(&v->ints);
    runtime_valuevec_init(&v->floats);
    runtime_valuevec_init(&v->strings);
}

void code_constant_pool_free(struct code_constant_pool *v)
{
    runtime_valuevec_free(&v->ints);
    runtime_valuevec_free(&v->floats);
    runtime_valuevec_free(&v->strings);
}

int code_constant_pool_push_int(struct code_constant_pool *v, value_int_t val)
{
    for (int i = 0; i < v->ints.len; i++) {
        if (v->ints.data[i].inum == val)
            return i;
    }

    struct runtime_value value;
    value.inum = val;
    runtime_valuevec_push(&v->ints, value);

    int new_idx = v->ints.len - 1;
    return new_idx;
}

int code_constant_pool_push_float(struct code_constant_pool *v, value_float_t val)
{
    for (int i = 0; i < v->floats.len; i++) {
        if (v->floats.data[i].fpnum == val)
            return i;
    }

    struct runtime_value value;
    value.fpnum = val;
    runtime_valuevec_push(&v->floats, value);

    int new_idx = v->floats.len - 1;
    return new_idx;
}

int code_constant_pool_push_string(struct code_constant_pool *v, const char *val)
{
    for (int i = 0; i < v->strings.len; i++) {
        struct runtime_value s = runtime_valuevec_get(&v->strings, i);
        if (runtime_string_compare_cstr(s.string, val) == 0)
            return i;
    }

    struct runtime_value value;
    value.string = runtime_string_new(val);
    runtime_valuevec_push(&v->strings, value);

    int new_idx = v->strings.len - 1;
    return new_idx;
}

struct runtime_value code_constant_pool_get_int(const struct code_constant_pool *v, int id)
{
    return runtime_valuevec_get(&v->ints, id);
}

struct runtime_value code_constant_pool_get_float(const struct code_constant_pool *v, int id)
{
    return runtime_valuevec_get(&v->floats, id);
}

struct runtime_value code_constant_pool_get_string(const struct code_constant_pool *v, int id)
{
    return runtime_valuevec_get(&v->strings, id);
}

int code_constant_pool_get_int_count(const struct code_constant_pool *v)
{
    return v->ints.len;
}

int code_constant_pool_get_float_count(const struct code_constant_pool *v)
{
    return v->floats.len;
}

int code_constant_pool_get_string_count(const struct code_constant_pool *v)
{
    return v->strings.len;
}
