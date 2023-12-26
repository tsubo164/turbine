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
    case TY::PTR: return "*";
    case TY::ANY: return "any";
    }

    ERROR_NO_CASE(kind);
    return nullptr;
}

std::string TypeString(const Type *type)
{
    std::string s;

    for (const Type *t = type; t; t = t->underlying)
        s += type_kind_string(t->kind);

    return s;
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

std::ostream &operator<<(std::ostream &os, const Type *type)
{
    return os << TypeString(type);
}
