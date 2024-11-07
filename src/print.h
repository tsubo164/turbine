#ifndef PRINT_H
#define PRINT_H

#include <stdbool.h>

struct Module;
struct Token;
struct Scope;
struct parser_expr;

void PrintToken(const struct Token *token, bool format);
void PrintProg(const struct Module *mod);
void PrintScope(const struct Scope *sc);
void PrintExpr(const struct parser_expr *e);

#endif // _H
