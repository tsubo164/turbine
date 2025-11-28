#ifndef BUILTIN_H
#define BUILTIN_H

#include "parser_type.h"

struct parser_scope;

void define_builtin_functions(struct parser_scope *builtin, struct parser_type_pool *type_pool);

#endif /* _H */
