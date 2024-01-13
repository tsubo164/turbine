#ifndef LEXER_H
#define LEXER_H

enum TK {
    TK_UNKNOWN = 0,
    // factor
    //TK_INTLIT,
    //TK_FLTLIT,
    TK_STRLIT,
    TK_IDENT,
    // operator
    TK_EQ,
    TK_PLUSEQ,
    TK_MINUSEQ,
    TK_STAREQ,
    TK_SLASHEQ,
    TK_PERCENTEQ,
    TK_EQ2,
    TK_EXCLEQ,
    TK_EXCL,
    TK_CARET,
    TK_TILDA,
    TK_LT2,
    TK_GT2,
    TK_LT,
    TK_GT,
    TK_LTE,
    TK_GTE,
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_BAR,
    TK_BAR2,
    TK_AMP,
    TK_AMP2,
    TK_PERIOD,
    TK_PLUS2,
    TK_MINUS2,
    TK_HASH,
    TK_HASH2,
    // keyword
    TK_NIL,
    TK_TRUE,
    TK_FALSE,
    TK_BOOL,
    TK_INT,
    TK_FLOAT,
    TK_STRING,
    TK_IF,
    TK_OR,
    TK_ELSE,
    TK_FOR,
    TK_BREAK,
    TK_CONTINUE,
    TK_SWITCH,
    TK_CASE,
    TK_DEFAULT,
    TK_RETURN,
    TK_NOP,
    // separator
    TK_MINUS3,
    TK_COMMA,
    TK_SEMICOLON,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACK,
    TK_RBRACK,
    TK_BLOCKBEGIN,
    TK_BLOCKEND,
    TK_NEWLINE,
    // special var
    TK_CALLER_LINE,
    //TK_EOF,
};

typedef struct Pos {
    int x, y;
} Pos;

typedef struct Token {
    int kind;
    Pos pos;
    union {
        long ival;
        double fval;
        bool has_escseq;
        const char *sval;
    };
    struct Token *prev;
    struct Token *next;
} Token;

const Token *Tokenize(const char *src);
void PrintToken(const Token *token, bool format);
const char *TokenKindString(int kind);

#endif // _H
