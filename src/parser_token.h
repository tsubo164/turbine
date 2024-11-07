#ifndef PARSER_TOKEN_H
#define PARSER_TOKEN_H

#include <stdbool.h>

enum parser_token_kind {
    TOK_NUL,
    TOK_keyword_begin,
    // type
    TOK_NIL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_BOOL,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
    // stmt
    TOK_IF,
    TOK_FOR,
    TOK_ELSE,
    TOK_BREAK,
    TOK_CONTINUE,
    TOK_SWITCH,
    TOK_CASE,
    TOK_DEFAULT,
    TOK_RETURN,
    TOK_NOP,
    TOK_EXPR,
    TOK_BLOCK,
    // special
    TOK_CALLER_LINE,
    TOK_keyword_end,
    // identifier
    TOK_FIELD,
    TOK_IDENT,
    TOK_FUNC,
    TOK_VAR,
    // literal
    TOK_NILLIT,
    TOK_BOOLLIT,
    TOK_INTLIT,
    TOK_FLOATLIT,
    TOK_STRINGLIT,
    TOK_FUNCLIT,
    TOK_ARRAYLIT,
    TOK_STRUCTLIT,
    // separator
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_LBRACK,
    TOK_RBRACK,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_COLON2,
    TOK_BLOCKBEGIN,
    TOK_BLOCKEND,
    TOK_DASH3,
    TOK_DOT,
    TOK_COMMA,
    TOK_HASH,
    TOK_HASH2,
    TOK_NEWLINE,
    // binary
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_REM,
    // relational
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE,
    // bitwise
    TOK_SHL,
    TOK_SHR,
    TOK_OR,
    TOK_XOR,
    TOK_AND,
    TOK_LOR,
    TOK_LAND,
    // array, struct, func
    TOK_SELECT,
    TOK_INDEX,
    TOK_CALL,
    // unary
    TOK_LNOT,
    TOK_POS,
    TOK_NEG,
    TOK_ADR,
    TOK_DRF,
    TOK_NOT,
    TOK_INC,
    TOK_DEC,
    TOK_CONV,
    // assign
    TOK_ASSN,
    TOK_AADD,
    TOK_ASUB,
    TOK_AMUL,
    TOK_ADIV,
    TOK_AREM,
    TOK_INIT,
    TOK_ELEMENT,
    // eof
    TOK_EOF
};

struct KindInfo {
    int kind;
    const char *str;
    char type;
};

struct parser_pos {
    int x, y;
};

struct parser_token {
    int kind;
    struct parser_pos pos;

    union {
        long ival;
        double fval;
        const char *sval;
    };
    bool has_escseq;

    struct parser_token *prev;
    struct parser_token *next;
};

const struct parser_token *Tokenize(const char *src);
const struct KindInfo *LookupKindInfo(int kind);
const char *TokenString(int kind);

#endif /* _H */
