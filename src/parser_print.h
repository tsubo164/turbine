#ifndef PARSER_PRINT_H
#define PARSER_PRINT_H

#include <stdbool.h>

struct Module;
struct parser_token;
struct Scope;
struct parser_expr;

void parser_print_token(const struct parser_token *token, bool format);
void parser_print_prog(const struct Module *mod);
void parser_print_scope(const struct Scope *sc);
void parser_print_expr(const struct parser_expr *e);

#endif /* _H */
