#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

struct FuncType;
struct Struct;
struct Table;
struct Module;

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

struct Type {
    enum TY kind;
    const struct Type *underlying;
    union {
        const struct FuncType *func_type;
        const struct Struct *strct;
        const struct Table *table;
        const struct Module *module;
        int len;
    };
};

struct Type *NewNilType();
struct Type *NewBoolType();
struct Type *NewIntType();
struct Type *NewFloatType();
struct Type *NewStringType();
struct Type *NewFuncType(struct FuncType *func_type);
struct Type *NewStructType(struct Struct *s);
struct Type *NewTableType(struct Table *t);
struct Type *NewModuleType(struct Module *m);
struct Type *NewPtrType(const struct Type *underlying);
struct Type *NewArrayType(int len, const struct Type *underlying);
struct Type *NewAnyType();

bool IsNil(const struct Type *t);
bool IsBool(const struct Type *t);
bool IsInt(const struct Type *t);
bool IsFloat(const struct Type *t);
bool IsString(const struct Type *t);
bool IsFunc(const struct Type *t);
bool IsStruct(const struct Type *t);
bool IsTable(const struct Type *t);
bool IsModule(const struct Type *t);
bool IsPtr(const struct Type *t);
bool IsArray(const struct Type *t);
bool IsAny(const struct Type *t);

int SizeOf(const struct Type *t);
bool MatchType(const struct Type *t1, const struct Type *t2);
struct Type *DuplicateType(const struct Type *t);
const char *TypeString(const struct Type *type);

#endif // _H
