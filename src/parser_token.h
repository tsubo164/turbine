#ifndef PARSER_TOKEN_H
#define PARSER_TOKEN_H

#include "value_types.h"
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
    TOK_STRUCT,
    TOK_ENUM,
    TOK_VEC,
    TOK_MAP,
    TOK_SET,
    TOK_STACK,
    TOK_QUEUE,
    TOK_IF,
    TOK_ELSE,
    TOK_ELIF,
    TOK_WHILE,
    TOK_FOR,
    TOK_IN,
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
    TOK_BLOCKBEGIN,
    TOK_BLOCKEND,
    TOK_MINUS3,
    TOK_PERIOD,
    TOK_PERIOD2,
    TOK_COMMA,
    TOK_HASH,
    TOK_HASH2,
    TOK_NEWLINE,
    /* binary */
    TOK_PLUS,
    TOK_MINUS,
    TOK_ASTER,
    TOK_SLASH,
    TOK_PERCENT,
    /* relational */
    TOK_EQUAL2,
    TOK_EXCLAMEQ,
    TOK_LT,
    TOK_LTE,
    TOK_GT,
    TOK_GTE,
    /* bitwise */
    TOK_LT2,
    TOK_GT2,
    TOK_CARET,
    TOK_VBAR,
    TOK_VBAR2,
    TOK_AMPERSAND,
    TOK_AMPERSAND2,
    /* unary */
    TOK_EXCLAM,
    TOK_TILDE,
    /* assign */
    TOK_EQUAL,
    TOK_PLUSEQ,
    TOK_MINUSEQ,
    TOK_ASTEREQ,
    TOK_SLASHEQ,
    TOK_PERCENTEQ,
    TOK_LT2EQ,
    TOK_GT2EQ,
    TOK_CARETEQ,
    TOK_VBAREQ,
    TOK_AMPERSANDEQ,
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
        value_int_t ival;
        value_float_t fval;
        const char *sval;
    };
    bool has_escseq;

    struct parser_token *prev;
    struct parser_token *next;
};

struct parser_token *parser_tokenize(const char *src, const char *filename);
const char *parser_get_token_string(int kind);
void parser_free_tokens(struct parser_token *tokens);

#endif /* _H */
