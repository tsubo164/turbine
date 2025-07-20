#include "module_file.h"
#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_string.h"
#include "runtime_vec.h"
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

static int file_new(struct runtime_gc *gc, struct runtime_registers *regs)
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

        ret.string = runtime_string_new(gc, sb.data);
        data_strbuf_free(&sb);
    }

    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int file_write_text(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value path = regs->locals[0];
    struct runtime_value text = regs->locals[1];
    struct runtime_value ret = {0};

    {
        FILE *fp = fopen(runtime_string_get_cstr(path.string), "w");
        if (!fp) {
            /* TODO error */
            ret.inum = 0;
            regs->locals[0] = ret;
            return RESULT_FAIL;
        }

        const char *s = runtime_string_get_cstr(text.string);
        fprintf(fp, "%s", s);
        fclose(fp);
    }

    ret.inum = 1;
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int file_read_lines(struct runtime_gc *gc, struct runtime_registers *regs)
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
        struct runtime_vec *lines = runtime_vec_new(gc, VAL_STRING, 0);
        /* TODO consider making runtime_gc_vec_new() */
        runtime_gc_push_object(gc, (struct runtime_object*) lines);

        while (1) {
            int c = fgetc(fp);
            if (c == EOF) {
                if (data_strbuf_len(&sb) > 0) {
                    struct runtime_value line;
                    line.string = runtime_string_new(gc, sb.data);
                    runtime_vec_push(gc, lines, line);
                    data_strbuf_clear(&sb);
                }
                break;
            }

            data_strbuf_push(&sb, c);

            if (c == '\n') {
                struct runtime_value line;
                line.string = runtime_string_new(gc, sb.data);
                runtime_vec_push(gc, lines, line);
                data_strbuf_clear(&sb);
            }
        }
        fclose(fp);

        ret.vec = lines;
        data_strbuf_free(&sb);
    }

    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int file_write_lines(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value path = regs->locals[0];
    struct runtime_value lines = regs->locals[1];
    struct runtime_value ret = {0};

    {
        FILE *fp = fopen(runtime_string_get_cstr(path.string), "w");
        if (!fp) {
            /* TODO error */
            ret.inum = 0;
            regs->locals[0] = ret;
            return RESULT_FAIL;
        }

        struct runtime_vec *sv = lines.vec;
        int count = runtime_vec_len(sv);

        for (int i = 0; i < count; i++) {
            struct runtime_value line = runtime_vec_get(sv, i);
            const char *s = runtime_string_get_cstr(line.string);
            fprintf(fp, "%s", s);
        }
        fclose(fp);
    }

    ret.inum = 1;
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

int module_define_file(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, "_builtin", "file");
    struct parser_struct *file_struct = NULL;

    /* struct */
    {
        const char *name = "File";
        struct native_struct_field fields[] = {
            { "fd", parser_new_int_type() },
            { NULL },
        };

        file_struct = native_define_struct(mod->scope, name, fields);
    }
    /* function */
    {
        const char *name = "init";
        native_func_t fp = file_init;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "new";
        native_func_t fp = file_new;
        struct native_func_param params[] = {
            { "path", parser_new_string_type() },
            { "_ret", parser_new_struct_type(file_struct) },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "read_text";
        native_func_t fp = file_read_text;
        struct native_func_param params[] = {
            { "path", parser_new_string_type() },
            { "_ret", parser_new_string_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "write_text";
        native_func_t fp = file_write_text;
        struct native_func_param params[] = {
            { "path", parser_new_string_type() },
            { "text", parser_new_string_type() },
            { "_ret", parser_new_bool_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "read_lines";
        native_func_t fp = file_read_lines;
        struct native_func_param params[] = {
            { "path", parser_new_string_type() },
            { "_ret", parser_new_vec_type(parser_new_string_type()) },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "write_lines";
        native_func_t fp = file_write_lines;
        struct native_func_param params[] = {
            { "path",  parser_new_string_type() },
            { "lines", parser_new_vec_type(parser_new_string_type()) },
            { "_ret",  parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
