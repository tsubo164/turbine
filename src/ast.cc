#include "ast.h"
#include <iostream>

Node::Node(int node_kind) : kind(node_kind)
{
}

Node::~Node()
{
    delete lhs;
    delete rhs;
}

Node *NewNode(int kind)
{
    return new Node(kind);
}

void DeleteTree(Node *tree)
{
    delete tree;
}

static long eval(const Node *node)
{
    switch (node->kind) {

    case NOD_INTNUM:
        return node->ival;

    case NOD_ADD:
        {
            const long l = eval(node->lhs);
            const long r = eval(node->rhs);
            return l + r;
        }

    default:
        return 0;
    }
}

long EvalTree(const Node *tree)
{
    return eval(tree);
}

static const char *kind_string(int kind)
{
#define N(kind) case kind: return #kind;
    switch (kind) {
    N(NOD_NOP);
    N(NOD_INTNUM);
    N(NOD_ASSIGN);
    N(NOD_ADD);
    default: return "???";
    }
#undef N
}

static void print_recursive(const Node *node, int depth)
{
    if (!node)
        return;

    for (int i = 0; i < depth; i++)
        printf("  ");
    printf("%d. ", depth);

    printf("%s", kind_string(node->kind));

    switch (node->kind) {

    case NOD_INTNUM:
        printf(" %ld", node->ival);
        break;

    case NOD_ASSIGN:
        break;
    case NOD_ADD:
        break;

    case NOD_IDENT:
        printf(" %ld", node->ival);
        break;

    default:
        break;
    }
    printf("\n");

    print_recursive(node->lhs, depth + 1);
    print_recursive(node->rhs, depth + 1);
}

void PrintTree(const Node *tree)
{
    print_recursive(tree, 0);
}
