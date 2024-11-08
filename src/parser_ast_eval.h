#ifndef PARSER_AST_EVAL_H
#define PARSER_AST_EVAL_H

#include "parser_ast.h"
#include <stdbool.h>
#include <stdint.h>

bool parser_ast_is_global(const struct parser_expr *e);
bool parser_ast_is_mutable(const struct parser_expr *e);

bool parser_eval_expr(const struct parser_expr *e, int64_t *result);
bool parser_eval_addr(const struct parser_expr *e, int *result);

#endif /* _H */
