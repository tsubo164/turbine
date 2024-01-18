#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#define NALLOC(n,type) ((type*) calloc((n),sizeof(type)))
#define CALLOC(type) NALLOC(1,type)


// codegen
struct Bytecode;
struct Prog;

void SetOptimize(bool enable);
void GenerateCode(struct Bytecode *code, const struct Prog *prog);


// string interning
const char *intern(const char *str);


#endif // _H
