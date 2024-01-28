#ifndef BUILTIN_H
#define BUILTIN_H

struct Scope;
struct Prog;

void DefineBuiltinFuncs(struct Prog *prog, struct Scope *builtin);

#endif // _H
