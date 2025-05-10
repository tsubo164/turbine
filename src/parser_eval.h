#ifndef PARSER_EVAL_H
#define PARSER_EVAL_H

#include "parser_ast.h"
#include <stdbool.h>

bool parser_eval_expr(const struct parser_expr *e, value_int_t *result);

#endif /* _H */
