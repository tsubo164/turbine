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

static struct set_node *new_node(struct runtime_value val)
{
    struct set_node *n;
    n = calloc(1, sizeof(*n));
    n->val = val;
    return n;
}

static int comp_int(struct runtime_value val1, struct runtime_value val2)
{
    if (val1.inum > val2.inum)
        return -1;
    if (val1.inum < val2.inum)
        return 1;
    return 0;
}

static struct set_node *insert(struct runtime_set *s,
        struct set_node *node, struct runtime_value val)
{
    if (!node) {
        s->len++;
        return new_node(val);
    }

    int cmp = comp_int(node->val, val);

    if (cmp < 0)
        node->l = insert(s, node->l, val);
    else if (cmp > 0)
        node->r = insert(s, node->r, val);
    else
        return node;

    return node;
}

void runtime_set_add(struct runtime_set *s, struct runtime_value val)
{
    s->root = insert(s, s->root, val);
}
