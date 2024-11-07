#ifndef PARSER_AST_EVAL_H
#define PARSER_AST_EVAL_H

#include "ast.h"
#include <stdbool.h>
#include <stdint.h>

bool IsGlobal(const struct Expr *e);
bool IsMutable(const struct Expr *e);

int Addr(const struct Expr *e);
bool EvalExpr(const struct Expr *e, int64_t *result);
bool EvalAddr(const struct Expr *e, int *result);

#endif /* _H */
