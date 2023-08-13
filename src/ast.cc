#include "ast.h"

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
