#include "module_math.h"
#include "native_function.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_gc.h"
#include "data_intern.h"

#include <stdio.h>
#include <math.h>

/*
     double acosh(double)
     double asinh(double)
     double atanh(double)
*/

/* TODO currently the reg_count is zero. consider setting a number of
 * global variables to it. however also need to see if it is okay to
 * have different meaning than normal functions */

/* TODO anotehr way is to call designated function to init module,
 * in that case we might need to have dedicated instruction to init modules,
 * which might couple vm and module */
#define MATH_PI 3.141592653589793

bool is_close(double a, double b, double rel_tol, double abs_tol) {
    if (a == b) {
        return true;
    }

    double diff = fabs(a - b);
    return (diff <= abs_tol) || (diff <= rel_tol * fmax(fabs(a), fabs(b)));
}

static int math_init(struct runtime_gc *gc, struct runtime_registers *regs)
{
    if (regs->globals) {
        struct runtime_value pi  = { .fpnum = MATH_PI };
        struct runtime_value e   = { .fpnum = 2.718281828459045 };
        struct runtime_value inf = { .fpnum = INFINITY };
        regs->globals[0] = pi;
        regs->globals[1] = e;
        regs->globals[2] = inf;
    }

    return RESULT_SUCCESS;
}

static int math_isclose(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];
    struct runtime_value y = regs->locals[1];

    double rel_tol = 1e-9;
    double abs_tol = 1e-12;
    x.inum = is_close(x.fpnum, y.fpnum, rel_tol, abs_tol);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_pow(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];
    struct runtime_value y = regs->locals[1];

    x.fpnum = pow(x.fpnum, y.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_sqrt(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = sqrt(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

/* radian */
static int math_radians(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = x.fpnum * (MATH_PI / 180.0);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_degrees(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = x.fpnum * (180.0 / MATH_PI);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

/* trigonometric */
static int math_sin(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = sin(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_cos(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = cos(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_tan(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = tan(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_asin(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = asin(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_acos(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = acos(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_atan(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = atan(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_atan2(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];
    struct runtime_value y = regs->locals[1];

    x.fpnum = atan2(x.fpnum, y.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_sinh(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = sinh(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_cosh(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = cosh(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_tanh(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = tanh(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_asinh(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = asinh(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_acosh(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = acosh(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

static int math_atanh(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value x = regs->locals[0];

    x.fpnum = atanh(x.fpnum);
    regs->locals[0] = x;

    return RESULT_SUCCESS;
}

int module_define_math(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, "_builtin", "math");
    struct parser_struct *vec3_struct = NULL;

    /* global */
    {
        struct native_global_var gvars[] = {
            { "_PI_",  parser_new_float_type() },
            { "_E_",   parser_new_float_type() },
            { "_INF_", parser_new_float_type() },
            { NULL },
        };

        native_define_global_vars(mod->scope, gvars);
    }
    /* struct */
    {
        const char *name = "Vec3";
        struct native_struct_field fields[] = {
            { "x", parser_new_float_type() },
            { "y", parser_new_float_type() },
            { "z", parser_new_float_type() },
            { NULL },
        };

        vec3_struct = native_define_struct(mod->scope, name, fields);
    }
    /* function */
    {
        const char *name = "init";
        native_func_t fp = math_init;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "isclose";
        native_func_t fp = math_isclose;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "y",    parser_new_float_type() },
            { "_ret", parser_new_bool_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "pow";
        native_func_t fp = math_pow;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "y",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "sqrt";
        native_func_t fp = math_sqrt;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    /* trigonometric */
    {
        const char *name = "radians";
        native_func_t fp = math_radians;
        struct native_func_param params[] = {
            { "degree", parser_new_float_type() },
            { "_ret",   parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "degrees";
        native_func_t fp = math_degrees;
        struct native_func_param params[] = {
            { "degree", parser_new_float_type() },
            { "_ret",   parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "sin";
        native_func_t fp = math_sin;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "cos";
        native_func_t fp = math_cos;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "tan";
        native_func_t fp = math_tan;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "asin";
        native_func_t fp = math_asin;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "acos";
        native_func_t fp = math_acos;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "atan";
        native_func_t fp = math_atan;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "atan2";
        native_func_t fp = math_atan2;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "y",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    /* hyperbolic */
    {
        const char *name = "sinh";
        native_func_t fp = math_sinh;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "cosh";
        native_func_t fp = math_cosh;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "tanh";
        native_func_t fp = math_tanh;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "asinh";
        native_func_t fp = math_asinh;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "acosh";
        native_func_t fp = math_acosh;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "atanh";
        native_func_t fp = math_atanh;
        struct native_func_param params[] = {
            { "x",    parser_new_float_type() },
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
