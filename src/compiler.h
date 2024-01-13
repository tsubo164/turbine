#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "bytecode.h"
#include "ast.h"

#define NALLOC(n,type) ((type*) calloc((n),sizeof(type)))
//#define CALLOC(type) NALLOC(1,type)
#define CALLOC(type) (new type())


enum KindTag {
    T_NUL,
    /* type */
    T_NIL,
    T_BOL,
    T_INT,
    T_FLT,
    /* stmt */
    T_IF,
    T_FOR,
    T_ELS,
    T_BRK,
    T_CNT,
    T_SWT,
    T_CASE,
    T_DFLT,
    T_RET,
    T_NOP,
    T_EXPR,
    T_BLOCK,
    T_END_OF_KEYWORD,
    /* list */
    T_EXPRLIST,
    /* identifier */
    T_FIELD, //FIXME
    T_IDENT,
    T_FUNC,
    T_VAR,
    /* literal */
    T_NILLIT,
    T_BOLLIT,
    T_INTLIT,
    T_FLTLIT,
    T_STRLIT,
    /* separator */
    T_LPAREN,
    T_RPAREN,
    T_SEM,
    /* binop */
    T_ADD,
    T_SUB,
    T_MUL,
    T_DIV,
    T_REM,
    //
    T_EQ,
    T_NEQ,
    T_LT,
    T_LTE,
    T_GT,
    T_GTE,
    //
    T_SHL,
    T_SHR,
    T_OR,
    T_XOR,
    T_AND,
    T_LOR,
    T_LAND,
    //
    T_SELECT,
    T_INDEX,
    T_CALL,
    /* unary */
    T_LNOT,
    T_POS,
    T_NEG,
    T_ADR,
    T_DRF,
    T_NOT,
    T_INC,
    T_DEC,
    T_CONV,
    /* assign */
    T_ASSN,
    T_AADD,
    T_ASUB,
    T_AMUL,
    T_ADIV,
    T_AREM,
    /* eof */
    T_EOF
};

typedef struct TokInfo {
    int kind;
    const char *str;
    char type;
} TokInfo;

const TokInfo *find_tokinfo(int kind);

struct Expr;
struct Stmt;
struct FuncDef;
struct Prog;

bool IsNull(const Expr *e);
bool IsGlobal(const Expr *e);

int Addr(const Expr *e);
bool EvalExpr(const Expr *e, long *result);
bool EvalAddr(const Expr *e, int *result);

// print
void PrintProg(const Prog *p, int depth);


// codegen
class Bytecode;
void GenerateCode(Bytecode *code, const Prog *prog);


// str
#include <string_view>
int ConvertEscSeq(std::string_view s, std::string &converted);

const char *intern(const char *str);

#endif // _H
