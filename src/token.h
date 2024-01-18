#ifndef TOKEN_H
#define TOKEN_H

#include <stdbool.h>

enum KindTag {
    T_NUL,
    T_keyword_begin,
    // type
    T_NIL,
    T_TRU,
    T_FLS,
    T_BOL,
    T_INT,
    T_FLT,
    T_STR,
    // stmt
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
    // special
    T_CALLER_LINE,
    T_keyword_end,
    // list
    T_EXPRLIST,
    // identifier
    T_FIELD,
    T_IDENT,
    T_FUNC,
    T_VAR,
    // literal
    T_NILLIT,
    T_BOLLIT,
    T_INTLIT,
    T_FLTLIT,
    T_STRLIT,
    // separator
    T_LPAREN,
    T_RPAREN,
    T_LBRACK,
    T_RBRACK,
    T_SEM,
    T_BLOCKBEGIN,
    T_BLOCKEND,
    T_DASH3,
    T_DOT,
    T_COMMA,
    T_HASH,
    T_HASH2,
    T_NEWLINE,
    // binary
    T_ADD,
    T_SUB,
    T_MUL,
    T_DIV,
    T_REM,
    // relational
    T_EQ,
    T_NEQ,
    T_LT,
    T_LTE,
    T_GT,
    T_GTE,
    // bitwise
    T_SHL,
    T_SHR,
    T_OR,
    T_XOR,
    T_AND,
    T_LOR,
    T_LAND,
    // array, struct, func
    T_SELECT,
    T_INDEX,
    T_CALL,
    // unary
    T_LNOT,
    T_POS,
    T_NEG,
    T_ADR,
    T_DRF,
    T_NOT,
    T_INC,
    T_DEC,
    T_CONV,
    // assign
    T_ASSN,
    T_AADD,
    T_ASUB,
    T_AMUL,
    T_ADIV,
    T_AREM,
    // eof
    T_EOF
};


typedef struct KindInfo {
    int kind;
    const char *str;
    char type;
} KindInfo;


typedef struct Pos {
    int x, y;
} Pos;


typedef struct Token {
    int kind;
    Pos pos;
    union {
        long ival;
        double fval;
        const char *sval;
    };
    bool has_escseq;
    struct Token *prev;
    struct Token *next;
} Token;


const Token *Tokenize(const char *src);
const char *TokenKindString(int kind);
const KindInfo *LookupKindInfo(int kind);
void PrintToken(const Token *token, bool format);


#endif // _H
