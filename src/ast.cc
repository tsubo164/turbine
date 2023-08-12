#include "ast.h"

Node::Node(int node_kind) : kind(node_kind)
{
}

Node::~Node()
{
}

Node *NewNode(int kind)
{
    return new Node(kind);
}

void DeleteTree(Node *tree)
{
    delete tree;
}
