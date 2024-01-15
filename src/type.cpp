#include "type.h"
#include "compiler.h"
#include "scope.h"
#include "error.h"

Type *NewNilType()
{
    static Type t;
    t.kind = TY_NIL;
    return &t;
}

Type *NewBoolType()
{
    static Type t;
    t.kind = TY_BOOL;
    return &t;
}

Type *NewIntType()
{
    static Type t;
    t.kind = TY_INT;
    return &t;
}

Type *NewFloatType()
{
    static Type t;
    t.kind = TY_FLOAT;
    return &t;
}

Type *NewStringType()
{
    static Type t;
    t.kind = TY_STRING;
    return &t;
}

Type *NewClassType(Class *clss)
{
    Type *t = CALLOC(Type);
    t->kind = TY_CLASS;
    t->clss = clss;
    return t;
}

Type *NewFuncType(Func *func)
{
    Type *t = CALLOC(Type);
    t->kind = TY_FUNC;
    t->func = func;
    return t;
}

Type *NewPtrType(const Type *underlying)
{
    Type *t = CALLOC(Type);
    t->kind = TY_PTR;
    t->underlying = underlying;
    return t;
}

Type *NewArrayType(int len, Type *underlying)
{
    Type *t = CALLOC(Type);
    t->kind = TY_ARRAY;
    t->len = len;
    t->underlying = underlying;
    return t;
}

Type *NewAnyType()
{
    static Type t;
    t.kind = TY_ANY;
    return &t;
}

int SizeOf(const Type *t)
{
    if (IsArray(t))
        // use one value for length info
        return t->len + 1;
    else if (IsClass(t))
        return Size(t->clss);
    else
        return 1;
}

bool IsNil(const Type *t)     { return t->kind == TY_NIL; }
bool IsBool(const Type *t)    { return t->kind == TY_BOOL; }
bool IsInt(const Type *t)     { return t->kind == TY_INT; }
bool IsFloat(const Type *t)   { return t->kind == TY_FLOAT; }
bool IsString(const Type *t)  { return t->kind == TY_STRING; }
bool IsClass(const Type *t)   { return t->kind == TY_CLASS; }
bool IsFunc(const Type *t)    { return t->kind == TY_FUNC; }
bool IsPtr(const Type *t)     { return t->kind == TY_PTR; }
bool IsArray(const Type *t)   { return t->kind == TY_ARRAY; }
bool IsAny(const Type *t)     { return t->kind == TY_ANY; }

static const char *type_kind_string(TY kind)
{
    switch (kind) {
    case TY_NIL: return "nil";
    case TY_BOOL: return "bool";
    case TY_INT: return "int";
    case TY_FLOAT: return "float";
    case TY_STRING: return "string";
    case TY_CLASS: return "class";
    case TY_FUNC: return "#";
    case TY_PTR: return "*";
    case TY_ARRAY: return "[]";
    case TY_ANY: return "any";
    }

    UNREACHABLE;
    return nullptr;
}

const char *TypeString(const Type *type)
{
    const char *interned = "";

    for (const Type *t = type; t; t = t->underlying) {
        char buf[128] = {'\0'};

        if (t->kind == TY_ARRAY)
            sprintf(buf, "%s[%d]", interned, t->len);
        else
            sprintf(buf, "%s%s", interned, type_kind_string(t->kind));

        interned = intern(buf);
    }

    return interned;
}

bool MatchType(const Type *t1, const Type *t2)
{
    if (IsAny(t1) || IsAny(t2))
        return true;
    return t1->kind == t2->kind;
}

Type *DuplicateType(const Type *t)
{
    Type *dup = CALLOC(Type);
    *dup = *t;
    return dup;
}
