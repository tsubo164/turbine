#ifndef PARSER_H
#define PARSER_H

struct Token;
struct Prog;
struct Scope;

Prog *Parse(const char *src, const Token *tok, Scope *scope);

#endif // _H
