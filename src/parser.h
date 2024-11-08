#ifndef PARSER_H
#define PARSER_H

struct parser_module;
struct parser_scope;
struct parser_token;

struct parser_module *Parse(const char *src, const char *filename, const char *modulename,
        const struct parser_token *tok, struct parser_scope *scope);

#endif // _H
