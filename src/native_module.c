#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include <string.h>
#include <assert.h>

/* TODO consider removing parser_* call */
#include "parser_ast.h"

/* function */
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

/* struct */
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

/* enum */
struct parser_enum *native_define_enum(struct parser_scope *scope,
        const char *enumname,
        const struct native_enum_field *fields,
        const struct native_enum_value *values)
{
    const struct native_enum_field *field;
    const struct native_enum_value *value;
    struct parser_enum *enm;

    enm = parser_define_enum(scope, enumname);

    for (field = fields; field->name; field++) {
        struct parser_enum_field *f;
        f = parser_add_enum_field(enm, field->name);
        f->type = field->type;
    }

    int nfields = parser_get_enum_field_count(enm);
    int x = 0;

    for (value = values; value->sval; value++) {
        if (x == 0) {
            /* symbol */
            /* TODO consider removing parser_* call */
            struct parser_expr *expr = parser_new_stringlit_expr(value->sval);
            parser_add_enum_member(enm, value->sval);
            parser_add_enum_value_expr(enm, expr);
        }
        else {
            /* field */
            struct parser_expr *expr;
            const struct parser_enum_field *field;

            field = parser_get_enum_field(enm, x);

            if (parser_is_int_type(field->type)) {
                expr = parser_new_intlit_expr(value->ival);
            }
            else if (parser_is_float_type(field->type)) {
                expr = parser_new_floatlit_expr(value->fval);
            }
            else if (parser_is_string_type(field->type)) {
                expr = parser_new_stringlit_expr(value->sval);
            }
            else {
                assert("not a valid enum field type");
                expr = NULL;
            }

            parser_add_enum_value_expr(enm, expr);
        }
        x = (x + 1) % nfields;
    }
    assert(x == 0);

    return enm;
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
