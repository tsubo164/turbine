#include "interpreter.h"
#include "data_intern.h"
#include "builtin_module.h"
#include "parser_search_path.h"
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
#include "os.h"

#include <stdio.h>
#include <stdlib.h>

static void print_header(const char *title)
{
    printf("## %s\n", title);
    printf("---\n");
}

int64_t interpret_source(const char *text, const char *filename,
        const struct interpreter_option *opt)
{
    const struct parser_token *tok = NULL;
    struct parser_search_path paths = {0};
    struct parser_scope builtin = {0};
    struct code_bytecode code = {{0}};
    struct vm_cpu vm = {{0}};

    /* builtin modules */
    struct builtin_module_list builtin_modules;
    builtin_register_modules(&builtin_modules);

    /* search paths */
    char *current_directory = os_get_current_directory();
    char *filepath = os_path_join(current_directory, filename);
    char *filedir = os_dirname(filepath);
    parser_search_path_init(&paths, filedir);
    /* TODO consdier passing builtin modules to parser_parse() separately
     * instead of holding them in struct parser_search_path */
    parser_search_path_add_builtin_modules(&paths, &builtin_modules);

    /* builtin functions */
    define_builtin_functions(&builtin);

    /* tokenize */
    tok = parser_tokenize(text);

    /* print token */
    if (opt->print_token) {
        parser_print_token(tok, !opt->print_token_raw);
        if (!opt->print_tree && !opt->print_symbols && !opt->print_bytecode) {
            return 0;
        }
    }

    /* compile source */
    struct parser_module *prog;
    struct parser_source source;
    parser_source_init(&source, text, filename, data_string_intern(":main"));

    prog = parser_parse(tok, &builtin, &source, &paths);
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

    /* clean */
    parser_search_path_free(&paths);
    free(current_directory);
    free(filepath);
    free(filedir);

    return ret;
}
