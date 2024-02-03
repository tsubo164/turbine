#ifndef PRINT_H
#define PRINT_H

#include <stdbool.h>

struct Module;
struct Token;
struct Scope;

void PrintToken(const struct Token *token, bool format);
void PrintProg(const struct Module *mod);
void PrintScope(const struct Scope *sc);

#endif // _H
