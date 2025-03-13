#include "native_function.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include <string.h>

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

void native_declare_func_(struct parser_scope *scope,
        const char *modulename,
        const char *funcname,
        const struct native_func_param *params,
        native_func_t native_func)
{
    const struct native_func_param *param;
    struct parser_func *func;
    bool has_ret = false;

    func = parser_declare_native_func(scope, modulename, funcname, native_func);

    for (param = params; param->name; param++) {
        if (!strcmp(param->name, "_ret")) {
            parser_add_return_type(func, param->type);
            has_ret = true;
            break;
        }

        parser_declare_param(func, param->name, param->type);

        if (param->is_format)
            func->sig->has_format_param = true;
    }

    if (!has_ret)
        parser_add_return_type(func, parser_new_nil_type());
}
