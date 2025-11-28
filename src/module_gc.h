#ifndef MODULE_GC_H
#define MODULE_GC_H

#include "parser_type.h"

struct parser_scope;

int module_define_gc(struct parser_scope *scope, struct parser_type_pool *type_pool);

#endif /* _H */
