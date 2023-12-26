#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <string>

struct Class;

enum class TY {
    NIL,
    BOOL,
    INT,
    FLOAT,
    STRING,
    CLASS,
    PTR,
    ANY,
};

struct Type {
    Type(TY k)
        : kind(k), underlying(nullptr), clss(nullptr) {}
    const TY kind;
    const Type *underlying;
    const Class *clss;

    int Size() const;

    bool IsNil() const { return kind == TY::NIL; }
    bool IsBool() const { return kind == TY::BOOL; }
    bool IsInt() const { return kind == TY::INT; }
    bool IsFloat() const { return kind == TY::FLOAT; }
    bool IsString() const { return kind == TY::STRING; }
    bool IsClass() const { return kind == TY::CLASS; }
    bool IsPtr() const { return kind == TY::PTR; }
    bool IsAny() const { return kind == TY::ANY; }
};

bool MatchType(const Type *t1, const Type *t2);
const Type *DuplicateType(const Type *t);

std::string TypeString(const Type *type);
std::ostream &operator<<(std::ostream &os, const Type *type);

#endif // _H
