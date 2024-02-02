#ifndef PARSER_H
#define PARSER_H

struct Module;
struct Scope;
struct Token;

struct Module *Parse(const char *src, const char *filename, const char *modulename,
        const struct Token *tok, struct Scope *scope);

#endif // _H
