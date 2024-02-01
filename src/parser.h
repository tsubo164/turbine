#ifndef PARSER_H
#define PARSER_H

struct Module;
struct Scope;
struct Token;

struct Module *Parse(const char *src, const struct Token *tok, struct Scope *scope);

#endif // _H
