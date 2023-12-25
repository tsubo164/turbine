#include "type.h"
#include "scope.h"
#include "error.h"

int Type::Size() const
{
    if (IsClass())
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
    case TY::ANY: return "any";
    }

    ERROR_NO_CASE(kind);
    return nullptr;
}

const char *TypeString(const Type *type)
{
    return type_kind_string(type->kind);
}

bool MatchType(const Type *t1, const Type *t2)
{
    if (t1->IsAny() || t2->IsAny())
        return true;
    return t1->kind == t2->kind;
}

const Type *DuplicateType(const Type *t)
{
    Type *dup = new Type(t->kind);
    dup->clss = t->clss;

    return dup;
}

std::ostream &operator<<(std::ostream &os, TY kind)
{
    return os << type_kind_string(kind);
}
