#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <string>

struct Class;
struct Func;

enum class TY {
    NIL,
    BOOL,
    INT,
    FLOAT,
    STRING,
    CLASS,
    FUNC,
    PTR,
    ARRAY,
    ANY,
};

struct Type {
    Type(TY k, const Type *under)
        : kind(k), underlying(under), clss(nullptr) {}
    Type(TY k)
        : kind(k), underlying(nullptr), clss(nullptr) {}
    const TY kind;
    const Type *underlying;
    const Class *clss;
    const Func *func;
    int len = 0;

    int Size() const;

    bool IsNil() const { return kind == TY::NIL; }
    bool IsBool() const { return kind == TY::BOOL; }
    bool IsInt() const { return kind == TY::INT; }
    bool IsFloat() const { return kind == TY::FLOAT; }
    bool IsString() const { return kind == TY::STRING; }
    bool IsClass() const { return kind == TY::CLASS; }
    bool IsFunc() const { return kind == TY::FUNC; }
    bool IsPtr() const { return kind == TY::PTR; }
    bool IsArray() const { return kind == TY::ARRAY; }
    bool IsAny() const { return kind == TY::ANY; }
};

Type *NewBoolType();
Type *NewFuncType(Func *func);
Type *NewPtrType(const Type *underlying);
Type *NewArrayType(int len, Type *underlying);

bool MatchType(const Type *t1, const Type *t2);
Type *DuplicateType(const Type *t);

std::string TypeString(const Type *t);
std::ostream &operator<<(std::ostream &os, const Type *t);

#endif // _H
