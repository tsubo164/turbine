#include "type.h"
#include "scope.h"
#include "error.h"

Type *NewNilType()
{
    static Type t(TY::NIL);
    return &t;
}

Type *NewBoolType()
{
    static Type t(TY::BOOL);
    return &t;
}

Type *NewIntType()
{
    static Type t(TY::INT);
    return &t;
}

Type *NewFloatType()
{
    static Type t(TY::FLOAT);
    return &t;
}

Type *NewStringType()
{
    static Type t(TY::STRING);
    return &t;
}

Type *NewFuncType(Func *func)
{
    Type *t = new Type(TY::FUNC);
    t->func = func;
    return t;
}

Type *NewPtrType(const Type *underlying)
{
    Type *t = new Type(TY::PTR);
    t->underlying = underlying;
    return t;
}

Type *NewArrayType(int len, Type *underlying)
{
    Type *t = new Type(TY::ARRAY);
    t->len = len;
    t->underlying = underlying;
    return t;
}

int Type::Size() const
{
    if (IsArray())
        // use one value for length info
        return len + 1;
    else if (IsClass())
        return clss->Size();
    else
        return 1;
}

static const char *type_kind_string(TY kind)
{
    switch (kind) {
    case TY::NIL: return "nil";
    case TY::BOOL: return "bool";
    case TY::INT: return "int";
    case TY::FLOAT: return "float";
    case TY::STRING: return "string";
    case TY::CLASS: return "class";
    case TY::FUNC: return "#";
    case TY::PTR: return "*";
    case TY::ARRAY: return "[]";
    case TY::ANY: return "any";
    }

    ERROR_NO_CASE(kind);
    return nullptr;
}

std::string TypeString(const Type *T)
{
    std::string s;

    for (const Type *t = T; t; t = t->underlying) {
        if (t->kind == TY::ARRAY)
            s += "[" + std::to_string(t->len) + "]";
        else
            s += type_kind_string(t->kind);
    }

    return s;
}

bool MatchType(const Type *t1, const Type *t2)
{
    if (t1->IsAny() || t2->IsAny())
        return true;
    return t1->kind == t2->kind;
}

Type *DuplicateType(const Type *t)
{
    Type *dup = new Type(t->kind);
    dup->underlying = t->underlying;
    dup->clss = t->clss;
    dup->func = t->func;
    return dup;
}

std::ostream &operator<<(std::ostream &os, const Type *t)
{
    return os << TypeString(t);
}
