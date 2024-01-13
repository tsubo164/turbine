#ifndef LEXER_H
#define LEXER_H

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
