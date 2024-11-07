#ifndef PRINT_H
#define PRINT_H

#include <stdbool.h>

struct Module;
struct parser_token;
struct Scope;
struct parser_expr;

void PrintToken(const struct parser_token *token, bool format);
void PrintProg(const struct Module *mod);
void PrintScope(const struct Scope *sc);
void PrintExpr(const struct parser_expr *e);

#endif // _H
