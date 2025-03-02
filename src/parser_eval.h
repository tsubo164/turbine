#ifndef PARSER_EVAL_H
#define PARSER_EVAL_H

#include "parser_ast.h"
#include <stdbool.h>
#include <stdint.h>

bool parser_eval_expr(const struct parser_expr *e, int64_t *result);

#endif /* _H */
