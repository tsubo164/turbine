#ifndef TYPE_H
#define TYPE_H

#include <iostream>

struct Class;

enum class TypeKind {
    Integer,
    Float,
    String,
    ClassType,
};
using TY = TypeKind;

struct Type {
    Type(TypeKind k)
        : kind(k), clss(nullptr) {}
    const TypeKind kind;
    const Class *clss;

    int Size() const;
};

const Type *PromoteType(const Type *t1, const Type *t2);

std::ostream &operator<<(std::ostream &os, TypeKind kind);

#endif // _H
