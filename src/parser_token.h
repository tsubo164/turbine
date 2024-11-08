#ifndef PARSER_TOKEN_H
#define PARSER_TOKEN_H

#include <stdbool.h>

enum parser_token_kind {
    TOK_ROOT,
    TOK_KEYWORD_BEGIN,
    /* keyword */
    TOK_NIL,
    TOK_TRUE,
    TOK_FALSE,
    TOK_BOOL,
    TOK_INT,
    TOK_FLOAT,
    TOK_STRING,
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
    /* special */
    TOK_CALLER_LINE,
    TOK_KEYWORD_END,
    /* identifier */
    TOK_IDENT,
    /* literal */
    TOK_INTLIT,
    TOK_FLOATLIT,
    TOK_STRINGLIT,
    /* separator */
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
    /* binary */
    TOK_ADD,
    TOK_SUB,
    TOK_MUL,
    TOK_DIV,
    TOK_REM,
    /* relational */
    TOK_EQ,
    TOK_NEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE,
    /* bitwise */
    TOK_SHL,
    TOK_SHR,
    TOK_OR,
    TOK_XOR,
    TOK_AND,
    TOK_LOR,
    TOK_LAND,
    /* unary */
    TOK_LNOT,
    TOK_NOT,
    TOK_INC,
    TOK_DEC,
    /* assign */
    TOK_ASSN,
    TOK_AADD,
    TOK_ASUB,
    TOK_AMUL,
    TOK_ADIV,
    TOK_AREM,
    /* eof */
    TOK_EOF
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

const struct parser_token *parser_tokenize(const char *src);
const char *parser_get_token_string(int kind);

#endif /* _H */
