#include "runtime_set.h"
#include <stdlib.h>

struct runtime_set *runtime_set_new(int64_t len)
{
    struct runtime_set *s;

    s = calloc(1, sizeof(*s));
    s->obj.kind = OBJ_SET;

    return s;
}

static void free_node(struct set_node *n)
{
    if (!n)
        return;
    free_node(n->l);
    free_node(n->r);
    free(n);
}

void runtime_set_free(struct runtime_set *s)
{
    if (!s)
        return;
    free_node(s->root);
    free(s);
}

int64_t runtime_set_len(const struct runtime_set *s)
{
    return s->len;
}
