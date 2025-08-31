#include "code_constant_pool.h"
#include "runtime_string.h"
#include <assert.h>

#define MIN_CAP 8

/* -------- */
void pool_init(struct code_constant_pool *v)
{
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

void push_const(struct code_constant_pool *v, struct code_constant c)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = c;
}

void pool_clear(struct code_constant_pool *v)
{
    free(v->data);
    pool_init(v);
}
/* -------- */

void code_constant_pool_init(struct code_constant_pool *v)
{
    runtime_valuevec_init(&v->ints);
    runtime_valuevec_init(&v->floats);
    runtime_valuevec_init(&v->strings);

    /* TODO consider making struct code_literal_table */
    data_strbuf_clear(&v->literal_types);

    /* TODO TEST */
    pool_init(v);
}

void code_constant_pool_free(struct code_constant_pool *v)
{
    /* free constant strings */
    value_int_t len = runtime_valuevec_len(&v->strings);
    for (value_int_t i = 0; i < len; i++) {
        struct runtime_value val = runtime_valuevec_get(&v->strings, i);
        runtime_string_free(NULL, val.string);
    }

    /* free constant vecs */
    runtime_valuevec_free(NULL, &v->ints);
    runtime_valuevec_free(NULL, &v->floats);
    runtime_valuevec_free(NULL, &v->strings);

    /* TODO consider making struct code_literal_table */
    {
        /* free literal strings */
        value_int_t len = runtime_valuevec_len(&v->literals);
        for (value_int_t i = 0; i < len; i++) {
            if (v->literal_types.data[i] == 's') {
                struct runtime_value val = runtime_valuevec_get(&v->literals, i);
                runtime_string_free(NULL, val.string);
            }
        }

        /* free literal vecs */
        runtime_valuevec_free(NULL, &v->literals);
        data_strbuf_free(&v->literal_types);
    }

    /* TODO TEST */
    /* free const strings */
    struct code_constant_pool *pool = v;
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_STRING)
            continue;
        struct runtime_value val = pool->data[i].val;
        runtime_string_free(NULL, val.string);
    }

    pool_clear(v);
}

int code_constant_pool_push_int(struct code_constant_pool *pool, value_int_t val)
{
    /* find */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_INT)
            continue;
        if (pool->data[i].val.inum == val)
            return i;
    }

    int new_idx = pool->len;
    struct runtime_value v = {.inum = val};
    struct code_constant c = {.val = v, .type = VAL_INT};

    push_const(pool, c);
    return new_idx;
}

int code_constant_pool_push_float(struct code_constant_pool *pool, value_float_t val)
{
    /* find */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_FLOAT)
            continue;
        if (pool->data[i].val.fpnum == val)
            return i;
    }

    int new_idx = pool->len;
    struct runtime_value v = {.fpnum = val};
    struct code_constant c = {.val = v, .type = VAL_FLOAT};

    push_const(pool, c);
    return new_idx;
}

int code_constant_pool_push_string(struct code_constant_pool *pool, const char *val)
{
    /* find */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_STRING)
            continue;
        struct runtime_value s = pool->data[i].val;
        if (runtime_string_compare_cstr(s.string, val) == 0)
            return i;
    }

    int new_idx = pool->len;
    struct runtime_value v = {.string = runtime_string_new(NULL, val)};
    struct code_constant c = {.val = v, .type = VAL_STRING};

    push_const(pool, c);
    return new_idx;
}

int code_constant_pool_get_count(const struct code_constant_pool *v)
{
    return v->len;
}

struct runtime_value code_constant_pool_get(const struct code_constant_pool *v, int id)
{
    assert(id >= 0 && id < v->len);
    return v->data[id].val;
}

int code_constant_pool_get_type(const struct code_constant_pool *v, int id)
{
    assert(id >= 0 && id < v->len);
    return v->data[id].type;
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

    runtime_valuevec_push(NULL, &v->literals, value);
    data_strbuf_push(&v->literal_types, 'i');

    return new_idx;
}

int code_constant_pool_push_literal_float(struct code_constant_pool *v, value_float_t val)
{
    struct runtime_value value = {.fpnum = val};
    int new_idx = v->literals.len;

    runtime_valuevec_push(NULL, &v->literals, value);
    data_strbuf_push(&v->literal_types, 'f');

    return new_idx;
}

int code_constant_pool_push_literal_string(struct code_constant_pool *v, const char *val)
{
    struct runtime_value value = {.string = runtime_string_new(NULL, val)};
    int new_idx = v->literals.len;

    runtime_valuevec_push(NULL, &v->literals, value);
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

bool code_constant_pool_is_literal_float(const struct code_constant_pool *v, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(v));
    return v->literal_types.data[id] == 'f';
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
