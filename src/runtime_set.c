#include "runtime_set.h"
#include <stdlib.h>
#include <stdio.h>

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
    n->height = 1;
    return n;
}

static int max(int a, int b)
{
    return a < b ? b : a;
}

static int height(const struct set_node *n)
{
    return n ? n->height : 0;
}

static int balance_factor(const struct set_node *n)
{
    return n ? height(n->r) - height(n->l) : 0;
}

static void update_height(struct set_node *n)
{
    n->height = 1 + max(height(n->l), height(n->r));
}

static struct set_node *rotate_left(struct set_node *pivot)
{
    struct set_node *p = pivot;
    struct set_node *r = p->r;
    struct set_node *rl = r->l;

    r->l = p;
    p->r = rl;

    update_height(p);
    update_height(r);

    return r;
}

static struct set_node *rotate_right(struct set_node *pivot)
{
    struct set_node *p = pivot;
    struct set_node *l = p->l;
    struct set_node *lr = l->r;

    l->r = p;
    p->l = lr;

    update_height(p);
    update_height(l);

    return l;
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

    int bf = 0;
    int cmp = comp_int(node->val, val);

    if (cmp < 0)
        node->l = insert(s, node->l, val);
    else if (cmp > 0)
        node->r = insert(s, node->r, val);
    else
        return node;

    update_height(node);
    bf = balance_factor(node);

    /* LL */
    if (bf < -1 && comp_int(node->l->val, val) < 0)
        return rotate_right(node);

    /* RR */
    if (bf >  1 && comp_int(node->r->val, val) > 0)
        return rotate_left(node);

    /* LR */
    if (bf < -1 && comp_int(node->l->val, val) > 0) {
        node->l = rotate_left(node->l);
        return rotate_right(node);
    }

    /* RL */
    if (bf >  1 && comp_int(node->r->val, val) < 0) {
        node->r = rotate_right(node->r);
        return rotate_left(node);
    }

    return node;
}

void runtime_set_add(struct runtime_set *s, struct runtime_value val)
{
    s->root = insert(s, s->root, val);
}

bool runtime_set_contains(const struct runtime_set *s, struct runtime_value key)
{
    struct set_node *node = s->root;

    while (node) {
        int cmp = comp_int(node->val, key);

        if (cmp < 0)
            node = node->l;
        else if (cmp > 0)
            node = node->r;
        else
            return true;
    }

    return false;
}


static void print_tree(const struct set_node *n, int depth)
{
    if (!n)
        return;
    printf( "%*s%lld\n", depth * 2, "", n->val.inum);
    print_tree(n->l, depth + 1);
    print_tree(n->r, depth + 1);
}

void runtime_print_set_tree(const struct runtime_set *s)
{
    print_tree(s->root, 0);
}
