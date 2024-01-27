#ifndef PARSER_H
#define PARSER_H

typedef struct Token Token;
typedef struct Prog Prog;
typedef struct Scope Scope;

void Parse(const char *src, const Token *tok, Scope *scope, Prog *prog);

#endif // _H
