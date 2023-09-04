#ifndef TYPE_H
#define TYPE_H

enum class TypeKind {
    Integer,
    String,
};

using TY = enum TypeKind;

struct Type {
    Type(TypeKind Kind) : kind(Kind) {}
    const TypeKind kind;
};

#endif // _H
