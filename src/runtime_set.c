#include "runtime_set.h"
#include <stdlib.h>
#include <stdio.h>

struct runtime_set *runtime_set_new(int val_type, int64_t len)
{
    struct runtime_set *s;

    s = calloc(1, sizeof(*s));
    s->obj.kind = OBJ_SET;
    s->val_type = val_type;
    s->compare = runtime_get_compare_function(s->val_type);

    return s;
}

static void free_node(struct runtime_set_node *n)
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

static void connect_left(struct runtime_set_node *n, struct runtime_set_node *l)
{
    n->l = l;
    if (l) l->parent = n;
}

static void connect_right(struct runtime_set_node *n, struct runtime_set_node *r)
{
    n->r = r;
    if (r) r->parent = n;
}

static struct runtime_set_node *new_node(struct runtime_value val)
{
    struct runtime_set_node *n;
    n = calloc(1, sizeof(*n));
    n->val = val;
    n->height = 1;
    return n;
}

static int max(int a, int b)
{
    return a < b ? b : a;
}

static int height(const struct runtime_set_node *n)
{
    return n ? n->height : 0;
}

static int balance_factor(const struct runtime_set_node *n)
{
    return n ? height(n->l) - height(n->r) : 0;
}

static void update_height(struct runtime_set_node *n)
{
    n->height = 1 + max(height(n->l), height(n->r));
}

static struct runtime_set_node *left_rotate(struct runtime_set_node *pivot)
{
    struct runtime_set_node *p = pivot;
    struct runtime_set_node *r = p->r;
    struct runtime_set_node *rl = r->l;

    connect_left(r, p);
    connect_right(p, rl);

    update_height(p);
    update_height(r);

    return r;
}

static struct runtime_set_node *right_rotate(struct runtime_set_node *pivot)
{
    struct runtime_set_node *p = pivot;
    struct runtime_set_node *l = p->l;
    struct runtime_set_node *lr = l->r;

    connect_right(l, p);
    connect_left(p, lr);

    update_height(p);
    update_height(l);

    return l;
}

static struct runtime_set_node *insert_node(struct runtime_set *s,
        struct runtime_set_node *node, struct runtime_value val)
{
    if (!node) {
        struct runtime_set_node *n;
        s->len++;
        n = new_node(val);
        if (!s->min)
            s->min = n;
        else
            s->min = s->compare(n->val, s->min->val) < 0 ? n : s->min;
        return n;
    }

    int cmp = s->compare(node->val, val);

    if (cmp > 0)
        connect_left(node, insert_node(s, node->l, val));
    else if (cmp < 0)
        connect_right(node, insert_node(s, node->r, val));
    else
        return node;

    update_height(node);
    int bf = balance_factor(node);

    /* LL */
    if (bf >  1 && s->compare(node->l->val, val) > 0)
        return right_rotate(node);

    /* RR */
    if (bf < -1 && s->compare(node->r->val, val) < 0)
        return left_rotate(node);

    /* LR */
    if (bf >  1 && s->compare(node->l->val, val) < 0) {
        connect_left(node, left_rotate(node->l));
        return right_rotate(node);
    }

    /* RL */
    if (bf < -1 && s->compare(node->r->val, val) > 0) {
        connect_right(node, right_rotate(node->r));
        return left_rotate(node);
    }

    return node;
}

struct runtime_set_node *min_value_node(struct runtime_set_node *node)
{
    struct runtime_set_node *n = node;
    while (n->l)
        n = n->l;
    return n;
}

static struct runtime_set_node *remove_node(struct runtime_set *s,
        struct runtime_set_node *node, struct runtime_value val)
{
    if (!node)
        return NULL;

    int cmp = s->compare(node->val, val);

    if (cmp > 0)
        connect_left(node, remove_node(s, node->l, val));
    else if (cmp < 0)
        connect_right(node, remove_node(s, node->r, val));
    else {
        /* found val */
        if (node->l == NULL) {
            struct runtime_set_node *tmp = node->r;
            free(node);
            s->len--;
            return tmp;
        }
        if (node->r == NULL) {
            struct runtime_set_node *tmp = node->l;
            free(node);
            s->len--;
            return tmp;
        }
        struct runtime_set_node *tmp = min_value_node(node->r);
        node->val = tmp->val;
        connect_right(node, remove_node(s, node->r, tmp->val));
    }

    update_height(node);
    int bf = balance_factor(node);

    /* LL */
    if (bf >  1 && balance_factor(node->l) >= 0)
        return right_rotate(node);

    /* RR */
    if (bf < -1 && balance_factor(node->r) <= 0)
        return left_rotate(node);

    /* LR */
    if (bf >  1 && balance_factor(node->l) < 0) {
        connect_left(node, left_rotate(node->l));
        return right_rotate(node);
    }

    /* RL */
    if (bf < -1 && balance_factor(node->r) > 0) {
        connect_right(node, right_rotate(node->r));
        return left_rotate(node);
    }

    return node;
}

bool runtime_set_add(struct runtime_set *s, struct runtime_value val)
{
    int oldlen = runtime_set_len(s);
    s->root = insert_node(s, s->root, val);
    if (s->root)
        s->root->parent = NULL;
    return runtime_set_len(s) == oldlen + 1;
}

bool runtime_set_remove(struct runtime_set *s, struct runtime_value val)
{
    int oldlen = runtime_set_len(s);
    s->root = remove_node(s, s->root, val);
    if (s->root)
        s->root->parent = NULL;
    return runtime_set_len(s) == oldlen - 1;
}

bool runtime_set_contains(const struct runtime_set *s, struct runtime_value val)
{
    struct runtime_set_node *node = s->root;

    while (node) {
        int cmp = s->compare(node->val, val);

        if (cmp > 0)
            node = node->l;
        else if (cmp < 0)
            node = node->r;
        else
            return true;
    }

    return false;
}

struct runtime_set_node *runtime_set_node_begin(const struct runtime_set *s)
{
    return s->min;
}

struct runtime_set_node *runtime_set_node_next(const struct runtime_set_node *n)
{
    if (n->r) {
        return min_value_node(n->r);
    }
    else {
        /* walk up to parent */
        struct runtime_set_node *p = n->parent;
        while (p && p->r == n) {
            n = p;
            p = p->parent;
        }
        return p;
    }
}

static void print_tree(const struct runtime_set_node *n, int depth)
{
    if (!n)
        return;
    printf( "%*s%" PRIival "\n", depth * 2, "", n->val.inum);
    print_tree(n->l, depth + 1);
    print_tree(n->r, depth + 1);
}

void runtime_print_set_tree(const struct runtime_set *s)
{
    print_tree(s->root, 0);
}
