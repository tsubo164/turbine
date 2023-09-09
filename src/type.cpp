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
    case TY::Integer: return "Integer";
    case TY::Float: return "Float";
    case TY::String: return "String";
    case TY::ClassType: return "ClassType";

    default:
        ERROR_NO_CASE(kind);
        return nullptr;
    }
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

std::ostream &operator<<(std::ostream &os, TypeKind kind)
{
    return os << type_kind_string(kind);
}
