#ifndef TYPE_H
#define TYPE_H

#include <iostream>

struct Class;

enum class TY {
    NIL,
    BOOL,
    INT,
    FLOAT,
    STRING,
    CLASS,
    ANY,
};

struct Type {
    Type(TY k)
        : kind(k), clss(nullptr) {}
    const TY kind;
    const Class *clss;

    int Size() const;

    bool IsNil() const { return kind == TY::NIL; }
    bool IsBool() const { return kind == TY::BOOL; }
    bool IsInt() const { return kind == TY::INT; }
    bool IsFloat() const { return kind == TY::FLOAT; }
    bool IsString() const { return kind == TY::STRING; }
    bool IsClass() const { return kind == TY::CLASS; }
    bool IsAny() const { return kind == TY::ANY; }
};

bool MatchType(const Type *t1, const Type *t2);
const Type *DuplicateType(const Type *t);

const char *TypeString(const Type *type);
std::ostream &operator<<(std::ostream &os, TY kind);

#endif // _H
