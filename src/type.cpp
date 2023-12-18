#include "type.h"
#include "scope.h"
#include "error.h"

int Type::Size() const
{
    if (kind == TY::ClassType)
        return clss->Size();
    else
        return 1;
}

static const char *type_kind_string(TypeKind kind)
{
    switch (kind) {
    case TY::Bool: return "bool";
    case TY::Integer: return "int";
    case TY::Float: return "float";
    case TY::String: return "string";
    case TY::ClassType: return "class";

    default:
        ERROR_NO_CASE(kind);
        return nullptr;
    }
}

const char *TypeString(const Type *type)
{
    return type_kind_string(type->kind);
}

bool MatchType(const Type *t1, const Type *t2)
{
    return t1->kind == t2->kind;
}

const Type *PromoteType(const Type *t1, const Type *t2)
{
    if (t1->kind == t2->kind)
        return new Type(t1->kind);

    if (t1->kind == TY::Integer && t2->kind == TY::Float)
        return new Type(t2->kind);

    if (t1->kind == TY::Float && t2->kind == TY::Integer)
        return new Type(t1->kind);

    return new Type(TY::Integer);
}

const Type *DuplicateType(const Type *t)
{
    Type *dup = new Type(t->kind);
    dup->clss = t->clss;

    return dup;
}

std::ostream &operator<<(std::ostream &os, TypeKind kind)
{
    return os << type_kind_string(kind);
}
