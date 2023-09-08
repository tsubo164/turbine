#ifndef TYPE_H
#define TYPE_H

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

#endif // _H
