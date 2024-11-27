#include "builtin_module_math.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_function.h"
#include "runtime_gc.h"
#include "data_intern.h"

#include <stdio.h>
#include <math.h>

/* TODO currently the reg_count is zero. consider setting a number of
 * global variables to it. however also need to see if it is okay to
 * have different meaning than normal functions */

/* TODO anotehr way is to call designated function to init module,
 * in that case we might need to have dedicated instruction to init modules,
 * which might couple vm and module */
static int init(struct runtime_gc *gc,
        struct runtime_value *registers, int reg_count)
{
    if (registers) {
        struct runtime_value pi = { .fpnum = 3.141592653589793 };
        struct runtime_value e  = { .fpnum = 2.718281828459045 };
        registers[0] = pi;
        registers[1] = e;
    }

    return RESULT_SUCCESS;
}

static int math_sqrt(struct runtime_gc *gc,
        struct runtime_value *registers, int reg_count)
{
    struct runtime_value x = registers[0];

    x.fpnum = sqrt(x.fpnum);
    registers[0] = x;

    return RESULT_SUCCESS;
}

int builtin_define_module_math(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, ":builtin", "math");
    /* TODO consider passing this to `parser_define_module()`
     * or making wrapper function named `builtin_define_module()`
     * Making wrappers seems better */
    /* this may help calling init function in code generator easier
    mod->is_builtin = true;
    */

    {
        const char *name = "PI";
        const struct parser_type *type = parser_new_float_type();
        bool isglobal = true;

        parser_define_var(mod->scope, name, type, isglobal);
    }
    {
        const char *name = "E";
        const struct parser_type *type = parser_new_float_type();
        bool isglobal = true;

        parser_define_var(mod->scope, name, type, isglobal);
    }
    {
        const char *name = data_string_intern("sqrt");
        struct parser_func *func = parser_declare_builtin_func(mod->scope, name);

        parser_declare_param(func, "x", parser_new_float_type());

        func->return_type = parser_new_float_type();
        func->func_type = parser_make_func_type(func);
        func->native_func_ptr = (void*) math_sqrt;
    }
    {
        const char *name = data_string_intern("Vec3");
        struct parser_struct *strct = parser_define_struct(mod->scope, name);

        parser_add_field(strct, data_string_intern("x"), parser_new_float_type());
        parser_add_field(strct, data_string_intern("y"), parser_new_float_type());
        parser_add_field(strct, data_string_intern("z"), parser_new_float_type());
    }
    {
        const char *name = data_string_intern(":init_math");
        struct parser_func *func = parser_declare_builtin_func(mod->scope, name);

        func->return_type = parser_new_int_type();
        func->func_type = parser_make_func_type(func);
        func->native_func_ptr = (void*) init;
    }

    return 0;
}
