#ifndef PARSER_H
#define PARSER_H

struct Module;
struct Scope;
struct parser_token;

struct Module *Parse(const char *src, const char *filename, const char *modulename,
        const struct parser_token *tok, struct Scope *scope);

#endif // _H
