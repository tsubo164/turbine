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

struct exec_pass {
    bool tokenize;
    bool parse;
    bool generate;
    bool execute;
};

static struct exec_pass make_exec_pass(const struct interpreter_option *opt) {
    struct exec_pass pass = {0};

    bool has_print_option =
        opt->print_token ||
        opt->print_tree ||
        opt->print_symbols ||
        opt->print_bytecode;

    if (opt->print_token)    pass.tokenize = true;
    if (opt->print_tree)     pass.tokenize = pass.parse = true;
    if (opt->print_symbols)  pass.tokenize = pass.parse = true;
    if (opt->print_bytecode) pass.tokenize = pass.parse = pass.generate = true;

    if (!has_print_option) {
        pass.tokenize = pass.parse = pass.generate = pass.execute = true;
    }

    return pass;
}

static void print_header(const char *title)
{
    printf("## %s\n", title);
    printf("---\n");
}

int64_t interpret_source(const char *text, const struct interpreter_args *args,
        const struct interpreter_option *opt)
{
    struct parser_search_path paths = {0};
    struct parser_scope builtin = {0};

    /* exec passes */
    struct exec_pass pass = make_exec_pass(opt);

    /* builtin modules */
    struct builtin_module_list builtin_modules;
    builtin_register_modules(&builtin_modules);

    /* search paths */
    char *current_directory = os_get_current_directory();
    char *filepath = os_path_join(current_directory, args->filename);
    char *filedir = os_dirname(filepath);
    parser_search_path_init(&paths, filedir);
    /* TODO consdier passing builtin modules to parser_parse() separately
     * instead of holding them in struct parser_search_path */
    parser_search_path_add_builtin_modules(&paths, &builtin_modules);

    /* builtin functions */
    define_builtin_functions(&builtin);

    /* tokenize */
    struct parser_token *tok = NULL;
    if (pass.tokenize) {
        tok = parser_tokenize(text, args->filename);
    }

    /* print token */
    if (opt->print_token) {
        parser_print_token(tok, !opt->print_token_raw);
    }

    /* compile source */
    struct parser_module *prog = NULL;
    struct parser_source source = {0};
    if (pass.parse) {
        parser_source_init(&source, text, args->filename, data_string_intern("_main"));
        prog = parser_parse(tok, &builtin, &source, &paths);
        code_resolve_offset(prog);
    }

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
    struct code_bytecode code = {{0}};
    if (pass.generate) {
        code_generate(&code, prog);
    }

    /* print bytecode */
    if (opt->print_bytecode) {
        print_header("bytecode");
        code_print_bytecode(&code);
    }

    /* execute */
    int64_t ret = 0;
    if (pass.execute) {
        struct vm_cpu vm = {{0}};
        struct vm_args vargs;
        vargs.values = args->values;
        vargs.count = args->count;

        vm_enable_print_stack(&vm, opt->print_stack);
        vm_execute_bytecode(&vm, &code, &vargs);
        ret = vm_get_stack_top(&vm);
    }

    /* clean */
    parser_free_tokens(tok);

    parser_search_path_free(&paths);
    free(current_directory);
    free(filepath);
    free(filedir);

    return ret;
}
