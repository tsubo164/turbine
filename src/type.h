#ifndef TYPE_H
#define TYPE_H

#include <iostream>

struct Class;

enum class TypeKind {
    Nil,
    Bool,
    Integer,
    Float,
    String,
    ClassType,
    Any,
};
using TY = TypeKind;

struct Type {
    Type(TypeKind k)
        : kind(k), clss(nullptr) {}
    const TypeKind kind;
    const Class *clss;

    int Size() const;

    bool IsNil() const { return kind == TY::Nil; }
    bool IsBool() const { return kind == TY::Bool; }
    bool IsInt() const { return kind == TY::Integer; }
    bool IsFloat() const { return kind == TY::Float; }
    bool IsString() const { return kind == TY::String; }
    bool IsClass() const { return kind == TY::ClassType; }
    bool IsAny() const { return kind == TY::Any; }
};

bool MatchType(const Type *t1, const Type *t2);
const Type *PromoteType(const Type *t1, const Type *t2);
const Type *DuplicateType(const Type *t);

const char *TypeString(const Type *type);
std::ostream &operator<<(std::ostream &os, TypeKind kind);

#endif // _H
