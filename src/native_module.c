#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include <string.h>

void native_declare_func(struct parser_scope *scope,
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

struct parser_struct *native_define_struct(struct parser_scope *scope,
        const char *structname,
        const struct native_struct_field *fields)
{
    const struct native_struct_field *field;
    struct parser_struct *strct;

    strct = parser_define_struct(scope, structname);

    for (field = fields; field->name; field++) {
        parser_add_struct_field(strct, field->name, field->type);
    }

    return strct;
}

void native_define_global_vars(struct parser_scope *scope,
        const struct native_global_var *gvars)
{
    const struct native_global_var *gvar;

    for (gvar = gvars; gvar->name; gvar++) {
        bool isglobal = true;
        parser_define_var(scope, gvar->name, gvar->type, isglobal);
    }
}
