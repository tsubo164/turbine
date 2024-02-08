#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

typedef struct Struct Struct;
struct Table;
struct Module;
typedef struct Func Func;

enum TY {
    TY_NIL,
    TY_BOOL,
    TY_INT,
    TY_FLOAT,
    TY_STRING,
    TY_FUNC,
    TY_STRUCT,
    TY_TABLE,
    TY_MODULE,
    TY_PTR,
    TY_ARRAY,
    TY_ANY,
};

typedef struct Type {
    enum TY kind;
    const struct Type *underlying;
    union {
        struct Func *func;
        const struct Struct *strct;
        const struct Table *table;
        const struct Module *module;
        int len;
    };
} Type;

Type *NewNilType();
Type *NewBoolType();
Type *NewIntType();
Type *NewFloatType();
Type *NewStringType();
struct Type *NewFuncType(struct Func *f);
struct Type *NewStructType(struct Struct *s);
struct Type *NewTableType(struct Table *t);
struct Type *NewModuleType(struct Module *m);
Type *NewPtrType(const Type *underlying);
Type *NewArrayType(int len, Type *underlying);
Type *NewAnyType();

bool IsNil(const Type *t);
bool IsBool(const Type *t);
bool IsInt(const Type *t);
bool IsFloat(const Type *t);
bool IsString(const Type *t);
bool IsFunc(const Type *t);
bool IsStruct(const Type *t);
bool IsTable(const struct Type *t);
bool IsModule(const struct Type *t);
bool IsPtr(const Type *t);
bool IsArray(const Type *t);
bool IsAny(const Type *t);

int SizeOf(const Type *t);
bool MatchType(const Type *t1, const Type *t2);
Type *DuplicateType(const Type *t);
const char *TypeString(const Type *type);

#endif // _H
