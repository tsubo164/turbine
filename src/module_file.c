#include "module_file.h"
#include "native_function.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_string.h"
#include "runtime_gc.h"
#include "data_intern.h"
#include "data_strbuf.h"

#include <stdio.h>

/* TODO currently the reg_count is zero. consider setting a number of
 * global variables to it. however also need to see if it is okay to
 * have different meaning than normal functions */
/* TODO anotehr way is to call designated function to init module,
 * in that case we might need to have dedicated instruction to init modules,
 * which might couple vm and module */
static int file_init(struct runtime_gc *gc, struct runtime_registers *regs)
{
    return RESULT_SUCCESS;
}

static int file_read_text(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value path = regs->locals[0];
    struct runtime_value ret = {0};

    {
        FILE *fp = fopen(runtime_string_get_cstr(path.string), "r");
        if (!fp) {
            /* TODO error */
            return RESULT_FAIL;
        }

        struct data_strbuf sb = DATA_STRBUF_INIT;

        while (1) {
            int c = fgetc(fp);
            if (c == EOF)
                break;

            data_strbuf_push(&sb, c);
        }
        fclose(fp);

        ret.string = runtime_gc_string_new(gc, sb.data);
        data_strbuf_free(&sb);
    }

    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

int module_define_file(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, "_builtin", "file");
    /* TODO consider passing this to `parser_define_module()`
     * or making wrapper function named `builtin_define_module()`
     * Making wrappers seems better */
    /* this may help calling init function in code generator easier
    mod->is_builtin = true;
    */
    {
        const char *name = "init";
        native_func_t fp = file_init;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func_(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "read_text";
        native_func_t fp = file_read_text;
        struct native_func_param params[] = {
            { "path", parser_new_string_type() },
            { "_ret", parser_new_string_type() },
            { NULL },
        };

        native_declare_func_(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
