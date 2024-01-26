#ifndef TYPE_H
#define TYPE_H

#include <stdbool.h>

typedef struct Class Class;
typedef struct Table Table;
struct Module;
typedef struct Func Func;

enum TY {
    TY_NIL,
    TY_BOOL,
    TY_INT,
    TY_FLOAT,
    TY_STRING,
    TY_FUNC,
    TY_CLASS,
    TY_TABLE,
    TY_MODULE,
    TY_PTR,
    TY_ARRAY,
    TY_ANY,
};

typedef struct Type {
    enum TY kind;
    const struct Type *underlying;
    const Func *func;
    const Class *clss;
    const Table *table;
    const struct Module *module;
    int len;
} Type;

Type *NewTypeNil();
Type *NewTypeBool();
Type *NewTypeInt();
Type *NewTypeFloat();
Type *NewTypeString();
Type *NewTypeFunc(Func *func);
Type *NewTypeClass(Class *clss);
Type *NewTypeTable(Table *tab);
struct Type *NewTypeModule(struct Module *mod);
Type *NewTypePtr(const Type *underlying);
Type *NewTypeArray(int len, Type *underlying);
Type *NewTypeAny();

bool IsNil(const Type *t);
bool IsBool(const Type *t);
bool IsInt(const Type *t);
bool IsFloat(const Type *t);
bool IsString(const Type *t);
bool IsFunc(const Type *t);
bool IsClass(const Type *t);
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
