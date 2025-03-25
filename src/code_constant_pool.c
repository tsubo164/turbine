#include "code_constant_pool.h"
#include "runtime_string.h"
#include <assert.h>

void code_constant_pool_init(struct code_constant_pool *v)
{
    runtime_valuevec_init(&v->ints);
    runtime_valuevec_init(&v->floats);
    runtime_valuevec_init(&v->strings);

    /* TODO consider making struct code_literal_table */
    data_strbuf_clear(&v->literal_types);
}

void code_constant_pool_free(struct code_constant_pool *v)
{
    runtime_valuevec_free(&v->ints);
    runtime_valuevec_free(&v->floats);
    runtime_valuevec_free(&v->strings);

    /* TODO consider making struct code_literal_table */
    runtime_valuevec_free(&v->literals);
    data_strbuf_free(&v->literal_types);
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

int code_constant_pool_push_literal_int(struct code_constant_pool *v, value_int_t val)
{
    struct runtime_value value = {.inum = val};
    int new_idx = v->literals.len;

    runtime_valuevec_push(&v->literals, value);
    data_strbuf_push(&v->literal_types, 'i');

    return new_idx;
}

int code_constant_pool_push_literal_string(struct code_constant_pool *v, const char *val)
{
    struct runtime_value value = {.string = runtime_string_new(val)};
    int new_idx = v->literals.len;

    runtime_valuevec_push(&v->literals, value);
    data_strbuf_push(&v->literal_types, 's');

    return new_idx;
}

struct runtime_value code_constant_pool_get_literal(const struct code_constant_pool *v, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(v));
    return runtime_valuevec_get(&v->literals, id);
}

bool code_constant_pool_is_literal_int(const struct code_constant_pool *v, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(v));
    return v->literal_types.data[id] == 'i';
}

bool code_constant_pool_is_literal_string(const struct code_constant_pool *v, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(v));
    return v->literal_types.data[id] == 's';
}

int code_constant_pool_get_literal_count(const struct code_constant_pool *v)
{
    return runtime_valuevec_len(&v->literals);
}
