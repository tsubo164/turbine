#ifndef PARSER_PRINT_H
#define PARSER_PRINT_H

#include <stdbool.h>

struct parser_module;
struct parser_token;
struct parser_scope;
struct parser_expr;

void parser_print_token(const struct parser_token *token, bool format);
void parser_print_prog(const struct parser_module *mod);
void parser_print_scope(const struct parser_scope *sc);
void parser_print_expr(const struct parser_expr *e);

#endif /* _H */
