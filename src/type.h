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

    bool IsInteger() const { return kind == TY::Integer; }
    bool IsFloat() const { return kind == TY::Float; }
    bool IsString() const { return kind == TY::String; }
    bool IsClass() const { return kind == TY::ClassType; }
};

const Type *PromoteType(const Type *t1, const Type *t2);
const Type *DuplicateType(const Type *t);

std::ostream &operator<<(std::ostream &os, TypeKind kind);

#endif // _H
