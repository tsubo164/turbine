#ifndef AST_H
#define AST_H

enum NodeKind {
    NOD_NOP = 0,
    NOD_INTNUM,
};

struct Node {
    Node(int node_kind);
    ~Node();

    int kind = 0;
    long ival = 0;
};

Node *NewNode(int kind);
void DeleteTree(Node *tree);

#endif // _H
