#ifndef PARSER_AST_EVAL_H
#define PARSER_AST_EVAL_H

#include "ast.h"
#include <stdbool.h>
#include <stdint.h>

bool IsGlobal(const struct parser_expr *e);
bool IsMutable(const struct parser_expr *e);

int Addr(const struct parser_expr *e);
bool EvalExpr(const struct parser_expr *e, int64_t *result);
bool EvalAddr(const struct parser_expr *e, int *result);

#endif /* _H */
