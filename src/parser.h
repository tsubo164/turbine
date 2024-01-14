#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "compiler.h"
#include "scope.h"
#include "type.h"
#include "ast.h"
#include <cstdint>
#include <array>

Prog *Parse(const char *src, const Token *tok, Scope *scope);

#endif // _H
