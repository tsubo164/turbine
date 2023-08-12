#ifndef AST_H
#define AST_H

enum NodeKind {
    NOD_NOP = 0,
    NOD_INTNUM,
    NOD_ADD,
};

struct Node {
    Node(int node_kind);
    ~Node();

    int kind = 0;
    long ival = 0;

    union {
        Node *lhs = nullptr;
    };
    union {
        Node *rhs = nullptr;
    };
};

Node *NewNode(int kind);
void DeleteTree(Node *tree);

long EvalTree(const Node *tree);

#endif // _H
