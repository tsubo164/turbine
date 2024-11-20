#include "interpreter.h"
#include "data_intern.h"
#include "parser_symbol.h"
#include "parser_parse.h"
#include "parser_token.h"
#include "parser_print.h"
#include "parser_ast.h"
#include "code_bytecode.h"
#include "code_generate.h"
#include "code_print.h"
#include "builtin.h"
#include "vm_cpu.h"

#include <stdio.h>

static void print_header(const char *title)
{
    printf("## %s\n", title);
    printf("---\n");
}

int64_t interpret_source(const char *src, const char *filename,
        const struct interpreter_option *opt)
{
    const struct parser_token *tok = NULL;
    struct parser_scope builtin = {0};
    struct code_bytecode code = {{0}};
    struct vm_cpu vm = {{0}};

    /* builtin functions */
    DefineBuiltinFuncs(&builtin);

    /* tokenize */
    tok = parser_tokenize(src);

    /* print token */
    if (opt->print_token) {
        parser_print_token(tok, !opt->print_token_raw);
        if (!opt->print_tree && !opt->print_symbols && !opt->print_bytecode) {
            return 0;
        }
    }

    /* compile source */
    struct parser_module *prog;

    prog = parser_parse(src, filename, data_string_intern("_main"), tok, &builtin);
    code_resolve_offset(prog);

    /* print tree */
    if (opt->print_tree) {
        print_header("tree");
        parser_print_prog(prog);
    }

    /* print symbols */
    if (opt->print_symbols) {
        print_header("symbol");
        if (opt->print_symbols_all)
            parser_print_scope(&builtin);
        else
            parser_print_scope(prog->scope);
    }

    /* generate bytecode */
    code_generate(&code, prog);

    if (opt->print_bytecode) {
        print_header("bytecode");
        code_print_bytecode(&code);
    }

    /* execute bytecode */
    int64_t ret = 0;
    if (!opt->print_tree && !opt->print_symbols && !opt->print_bytecode) {
        vm_enable_print_stack(&vm, opt->print_stack);
        bm_execute_bytecode(&vm, &code);
        ret = vm_get_stack_top(&vm);
    }

    return ret;
}
