#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

typedef struct Class Class;
typedef struct Table Table;
typedef struct Func Func;

enum TY {
    TY_NIL,
    TY_BOOL,
    TY_INT,
    TY_FLOAT,
    TY_STRING,
    TY_CLASS,
    TY_TABLE,
    TY_FUNC,
    TY_PTR,
    TY_ARRAY,
    TY_ANY,
};

typedef struct Type {
    enum TY kind;
    const struct Type *underlying;
    const Class *clss;
    const Table *table;
    const Func *func;
    int len;
} Type;

Type *NewTypeNil();
Type *NewTypeBool();
Type *NewTypeInt();
Type *NewTypeFloat();
Type *NewTypeString();
Type *NewTypeClass(Class *clss);
Type *NewTypeTable(Table *tab);
Type *NewTypeFunc(Func *func);
Type *NewTypePtr(const Type *underlying);
Type *NewTypeArray(int len, Type *underlying);
Type *NewTypeAny();

bool IsNil(const Type *t);
bool IsBool(const Type *t);
bool IsInt(const Type *t);
bool IsFloat(const Type *t);
bool IsString(const Type *t);
bool IsClass(const Type *t);
bool IsTable(const Type *t);
bool IsFunc(const Type *t);
bool IsPtr(const Type *t);
bool IsArray(const Type *t);
bool IsAny(const Type *t);

int SizeOf(const Type *t);
bool MatchType(const Type *t1, const Type *t2);
Type *DuplicateType(const Type *t);
const char *TypeString(const Type *type);

#endif // _H
