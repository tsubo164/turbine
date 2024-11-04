#include "type.h"
#include "data_intern.h"
#include "error.h"
#include "scope.h"
#include "mem.h"
#include <stdio.h>

struct Type *NewNilType()
{
    static struct Type t;
    t.kind = TY_NIL;
    return &t;
}

struct Type *NewBoolType()
{
    static struct Type t;
    t.kind = TY_BOOL;
    return &t;
}

struct Type *NewIntType()
{
    static struct Type t;
    t.kind = TY_INT;
    return &t;
}

struct Type *NewFloatType()
{
    static struct Type t;
    t.kind = TY_FLOAT;
    return &t;
}

struct Type *NewStringType()
{
    static struct Type t;
    t.kind = TY_STRING;
    return &t;
}

struct Type *NewFuncType(struct FuncType *func_type)
{
    struct Type *t = CALLOC(struct Type);
    t->kind = TY_FUNC;
    t->func_type = func_type;
    return t;
}

struct Type *NewStructType(struct Struct *s)
{
    struct Type *t = CALLOC(struct Type);
    t->kind = TY_STRUCT;
    t->strct = s;
    return t;
}

struct Type *NewTableType(struct Table *tab)
{
    struct Type *t = CALLOC(struct Type);
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

struct Type *NewPtrType(const struct Type *underlying)
{
    struct Type *t = CALLOC(struct Type);
    t->kind = TY_PTR;
    t->underlying = underlying;
    return t;
}

struct Type *NewArrayType(int len, const struct Type *underlying)
{
    struct Type *t = CALLOC(struct Type);
    t->kind = TY_ARRAY;
    t->len = len;
    t->underlying = underlying;
    return t;
}

struct Type *NewAnyType()
{
    static struct Type t;
    t.kind = TY_ANY;
    return &t;
}

int SizeOf(const struct Type *t)
{
    if (IsArray(t))
        // use one value for length info
        return t->len + 1;
    else if (IsStruct(t))
        return t->strct->size;
    else
        return 1;
}

bool IsNil(const struct Type *t)     { return t->kind == TY_NIL; }
bool IsBool(const struct Type *t)    { return t->kind == TY_BOOL; }
bool IsInt(const struct Type *t)     { return t->kind == TY_INT; }
bool IsFloat(const struct Type *t)   { return t->kind == TY_FLOAT; }
bool IsString(const struct Type *t)  { return t->kind == TY_STRING; }
bool IsFunc(const struct Type *t)    { return t->kind == TY_FUNC; }
bool IsStruct(const struct Type *t)   { return t->kind == TY_STRUCT; }
bool IsTable(const struct Type *t)    { return t->kind == TY_TABLE; }
bool IsModule(const struct Type *t)   { return t->kind == TY_MODULE; }
bool IsPtr(const struct Type *t)     { return t->kind == TY_PTR; }
bool IsArray(const struct Type *t)   { return t->kind == TY_ARRAY; }
bool IsAny(const struct Type *t)     { return t->kind == TY_ANY; }

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

const char *TypeString(const struct Type *type)
{
    const char *interned = "";

    for (const struct Type *t = type; t; t = t->underlying) {
        char buf[128] = {'\0'};

        if (t->kind == TY_ARRAY) {
            sprintf(buf, "%s[%d]", interned, t->len);
        }
        else if (t->kind == TY_PTR) {
            sprintf(buf, "%s*", interned);
        }
        else if (t->kind == TY_STRUCT) {
            sprintf(buf, "%s%s", interned, t->strct->name);
        }
        else {
            sprintf(buf, "%s%s", interned, type_kind_string(t->kind));
        }

        interned = data_string_intern(buf);
    }

    return interned;
}

bool MatchType(const struct Type *t1, const struct Type *t2)
{
    if (IsAny(t1) || IsAny(t2))
        return true;
    return t1->kind == t2->kind;
}

struct Type *DuplicateType(const struct Type *t)
{
    struct Type *dup = CALLOC(struct Type);
    *dup = *t;
    return dup;
}
