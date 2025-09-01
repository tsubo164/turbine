#include "code_constant_pool.h"
#include "runtime_string.h"
#include <assert.h>

#define MIN_CAP 8

/* -------- */
void init_pool(struct code_constant_pool *pool)
{
    pool->data = NULL;
    pool->cap = 0;
    pool->len = 0;
}

void push_const(struct code_constant_pool *p, struct code_constant c)
{
    if (p->len == p->cap) {
        p->cap = p->cap < MIN_CAP ? MIN_CAP : 2 * p->cap;
        p->data = realloc(p->data, p->cap * sizeof(*p->data));
    }
    p->data[p->len++] = c;
}

void clear_pool(struct code_constant_pool *pool)
{
    free(pool->data);
    init_pool(pool);
}
/* -------- */

void code_constant_pool_init(struct code_constant_pool *pool)
{
    /* TODO consider making struct code_literal_table */
    data_strbuf_clear(&pool->literal_types);

    init_pool(pool);
}

void code_constant_pool_free(struct code_constant_pool *pool)
{
    /* TODO consider making struct code_literal_table */
    {
        /* free literal strings */
        value_int_t len = runtime_valuevec_len(&pool->literals);
        for (value_int_t i = 0; i < len; i++) {
            if (pool->literal_types.data[i] == 's') {
                struct runtime_value val = runtime_valuevec_get(&pool->literals, i);
                runtime_string_free(NULL, val.string);
            }
        }

        /* free literal vecs */
        runtime_valuevec_free(NULL, &pool->literals);
        data_strbuf_free(&pool->literal_types);
    }

    /* free const strings */
    for (int i = 0; i < pool->len; i++) {
        if (pool->data[i].type != VAL_STRING)
            continue;
        struct runtime_value val = pool->data[i].val;
        runtime_string_free(NULL, val.string);
    }

    clear_pool(pool);
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

int code_constant_pool_get_count(const struct code_constant_pool *pool)
{
    return pool->len;
}

struct runtime_value code_constant_pool_get(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < pool->len);
    return pool->data[id].val;
}

int code_constant_pool_get_type(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < pool->len);
    return pool->data[id].type;
}

int code_constant_pool_push_literal_int(struct code_constant_pool *pool, value_int_t val)
{
    struct runtime_value value = {.inum = val};
    int new_idx = pool->literals.len;

    runtime_valuevec_push(NULL, &pool->literals, value);
    data_strbuf_push(&pool->literal_types, 'i');

    return new_idx;
}

int code_constant_pool_push_literal_float(struct code_constant_pool *pool, value_float_t val)
{
    struct runtime_value value = {.fpnum = val};
    int new_idx = pool->literals.len;

    runtime_valuevec_push(NULL, &pool->literals, value);
    data_strbuf_push(&pool->literal_types, 'f');

    return new_idx;
}

int code_constant_pool_push_literal_string(struct code_constant_pool *pool, const char *val)
{
    struct runtime_value value = {.string = runtime_string_new(NULL, val)};
    int new_idx = pool->literals.len;

    runtime_valuevec_push(NULL, &pool->literals, value);
    data_strbuf_push(&pool->literal_types, 's');

    return new_idx;
}

struct runtime_value code_constant_pool_get_literal(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(pool));
    return runtime_valuevec_get(&pool->literals, id);
}

bool code_constant_pool_is_literal_int(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(pool));
    return pool->literal_types.data[id] == 'i';
}

bool code_constant_pool_is_literal_float(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(pool));
    return pool->literal_types.data[id] == 'f';
}

bool code_constant_pool_is_literal_string(const struct code_constant_pool *pool, int id)
{
    assert(id >= 0 && id < code_constant_pool_get_literal_count(pool));
    return pool->literal_types.data[id] == 's';
}

int code_constant_pool_get_literal_count(const struct code_constant_pool *pool)
{
    return runtime_valuevec_len(&pool->literals);
}
