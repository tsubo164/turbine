#ifndef TYPE_H
#define TYPE_H

enum class TypeKind {
    Integer,
    String,
    Class,
};

using TY = TypeKind;

struct Clss;

struct Type {
    Type(TypeKind Kind) : kind(Kind) {}
    const TypeKind kind;
    const Clss *clss;

    int Size() const
    {
        if (kind == TY::Class) {
            return clss->Size();
        }
        else {
            return 1;
        }
    }
};

#endif // _H
