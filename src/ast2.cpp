#include "compiler.h"

#include "ast.h"

static const TokInfo table[] = {
    { T_NUL,     "nul" },
    // type
    { T_NIL,     "nil" },
    { T_BOL,     "bool" },
    { T_INT,     "int" },
    { T_FLT,     "float" },
    // stmt
    { T_IF,      "if" },
    { T_FOR,     "for" },
    { T_ELS,     "or" },
    { T_BRK,     "break" },
    { T_CNT,     "continue" },
    { T_SWT,     "switch" },
    { T_CASE,    "case" },
    { T_DFLT,    "default" },
    { T_RET,     "return" },
    { T_NOP,     "nop" },
    { T_EXPR,    "expr" },
    { T_BLOCK,   "block" },
    { T_END_OF_KEYWORD,   "end_of_keyword" },
    // list
    { T_EXPRLIST, "expr_list" },
    // identifier
    { T_FIELD,   "field",  'v' },
    { T_IDENT,   "ident",  'v' },
    { T_FUNC,    "func",   'v' },
    { T_VAR,     "var",    'v' },
    // literal
    { T_NILLIT,  "nil_lit" },
    { T_BOLLIT,  "bool_lit",    'i' },
    { T_INTLIT,  "int_lit",     'i' },
    { T_FLTLIT,  "float_lit",   'f' },
    { T_STRLIT,  "string_lit",  's' },
    // separator
    { T_LPAREN,  "(" },
    { T_RPAREN,  ")" },
    { T_SEM,     ";" },
    // binop
    { T_ADD,     "+" },
    { T_SUB,     "-" },
    { T_MUL,     "*" },
    { T_DIV,     "/" },
    { T_REM,     "%" },
    //
    { T_EQ,      "==" },
    { T_NEQ,     "!=" },
    { T_LT,      "<" },
    { T_LTE,     "<=" },
    { T_GT,      ">" },
    { T_GTE,     ">=" },
    //
    { T_SHL,     "<<" },
    { T_SHR,     ">>" },
    { T_OR,      "|" },
    { T_XOR,     "^" },
    { T_AND,     "&" },
    { T_LOR,     "||" },
    { T_LAND,    "&&" },
    //
    { T_SELECT,  "." },
    { T_INDEX,   "[]" },
    { T_CALL,    "call" },
    // unary
    { T_LNOT,    "!" },
    { T_POS,     "+" },
    { T_NEG,     "-" },
    { T_ADR,     "&" },
    { T_DRF,     "*" },
    { T_NOT,     "~" },
    { T_INC,     "++" },
    { T_DEC,     "--" },
    { T_CONV,    "conversion" },
    // assign
    { T_ASSN,    "=" },
    { T_AADD,    "+=" },
    { T_ASUB,    "-=" },
    { T_AMUL,    "*=" },
    { T_ADIV,    "/=" },
    { T_AREM,    "%=" },
    // eof
    { T_EOF,     "eof" },
};

// make an array of size 1 if table covers all kinds
// other wise size -1 which leads to compile error
// to avoid missing string impl of tokens
#define MISSING_TOKEN_STRING_IMPL \
    (sizeof(table)/sizeof(table[0])==T_EOF+1?1:-1)
//static const int assert_impl[MISSING_TOKEN_STRING_IMPL] = {0};

const TokInfo *find_tokinfo(int kind)
{
    int N = sizeof(table)/sizeof(table[0]);
    int i;

    for (i = 0; i < N; i++) {
        if (kind == table[i].kind)
            return &table[i];
    }
    return &table[0];
}
