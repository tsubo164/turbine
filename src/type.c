#include "type.h"
#include "intern.h"
#include "error.h"
#include "scope.h"
#include "mem.h"
#include <stdio.h>

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

struct Type *NewFuncType(struct Func *f)
{
    Type *t = CALLOC(struct Type);
    t->kind = TY_FUNC;
    t->func = f;
    return t;
}

struct Type *NewStructType(struct Struct *s)
{
    Type *t = CALLOC(struct Type);
    t->kind = TY_STRUCT;
    t->strct = s;
    return t;
}

struct Type *NewTableType(struct Table *tab)
{
    Type *t = CALLOC(struct Type);
    t->kind = TY_TABLE;
    t->table = tab;
    return t;
}

struct Type *NewModuleType(struct Module *mod)
{
    struct Type *t = CALLOC(struct Type);
    t->kind = TY_MODULE;
    t->module = mod;
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
    else if (IsStruct(t))
        return t->strct->size;
    else
        return 1;
}

bool IsNil(const Type *t)     { return t->kind == TY_NIL; }
bool IsBool(const Type *t)    { return t->kind == TY_BOOL; }
bool IsInt(const Type *t)     { return t->kind == TY_INT; }
bool IsFloat(const Type *t)   { return t->kind == TY_FLOAT; }
bool IsString(const Type *t)  { return t->kind == TY_STRING; }
bool IsFunc(const Type *t)    { return t->kind == TY_FUNC; }
bool IsStruct(const struct Type *t)   { return t->kind == TY_STRUCT; }
bool IsTable(const struct Type *t)    { return t->kind == TY_TABLE; }
bool IsModule(const struct Type *t)   { return t->kind == TY_MODULE; }
bool IsPtr(const Type *t)     { return t->kind == TY_PTR; }
bool IsArray(const Type *t)   { return t->kind == TY_ARRAY; }
bool IsAny(const Type *t)     { return t->kind == TY_ANY; }

static const char *type_kind_string(enum TY kind)
{
    switch (kind) {
    case TY_NIL: return "nil";
    case TY_BOOL: return "bool";
    case TY_INT: return "int";
    case TY_FLOAT: return "float";
    case TY_STRING: return "string";
    case TY_FUNC: return "func";
    case TY_STRUCT: return "struct";
    case TY_TABLE: return "table";
    case TY_MODULE: return "module";
    case TY_PTR: return "*";
    case TY_ARRAY: return "[]";
    case TY_ANY: return "any";
    }

    UNREACHABLE;
    return NULL;
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

        interned = StrIntern(buf);
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
