#ifndef PARSER_H
#define PARSER_H

typedef struct Token Token;
typedef struct Prog Prog;
typedef struct Scope Scope;

Prog *Parse(const char *src, const Token *tok, Scope *scope);

#endif // _H
