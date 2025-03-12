#include "native_function.h"
#include "parser_symbol.h"
#include "parser_type.h"

void native_declare_func(struct parser_scope *scope,
        const char *name,
        const struct native_func_param *params,
        const struct parser_type *return_type,
        native_func_t native_func)
{
    const struct native_func_param *param;
    struct parser_func *func;

    func = parser_declare_builtin_func(scope, name);

    for (param = params; param->name; param++) {
        parser_declare_param(func, param->name, param->type);

        if (param->is_format)
            func->sig->has_format_param = true;
    }

    parser_add_return_type(func, return_type);
    func->native_func_ptr = native_func;
}
