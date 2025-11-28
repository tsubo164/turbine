#ifndef MODULE_FILE_H
#define MODULE_FILE_H

#include "parser_type.h"

struct parser_scope;

int module_define_file(struct parser_scope *scope, struct parser_type_pool *type_pool);

#endif /* _H */
