#include "interpreter.h"
#include "compile_context.h"
#include "data_intern.h"
#include "builtin_module.h"
#include "parser_search_path.h"
#include "parser_symbol.h"
#include "parser_parse.h"
#include "parser_token.h"
#include "parser_print.h"
#include "parser_error.h"
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
        opt->print_bytecode ||
        opt->print_stackmap;

    if (opt->print_token)    pass.tokenize = true;
    if (opt->print_tree)     pass.tokenize = pass.parse = true;
    if (opt->print_symbols)  pass.tokenize = pass.parse = pass.generate = true;
    if (opt->print_bytecode) pass.tokenize = pass.parse = pass.generate = true;
    if (opt->print_stackmap) pass.tokenize = pass.parse = pass.generate = true;

    if (!has_print_option) {
        pass.tokenize = pass.parse = pass.generate = pass.execute = true;
    }

    return pass;
}

static char *get_script_dir(const char *filename)
{
    char *curr_dir = os_get_current_directory();
    char *filepath = os_path_join(curr_dir, filename);
    char *file_dir = os_dirname(filepath);

    free(curr_dir);
    free(filepath);

    return file_dir;
}

static void print_header(const char *title)
{
    printf("## %s\n", title);
    printf("---\n");
}

static void print_tree(const struct parser_module *prog)
{
    print_header("tree");
    parser_print_prog(prog);
}

static void print_syms(const struct parser_scope *main, bool print_symbols_all)
{
    print_header("symbol");
    if (print_symbols_all)
        parser_print_scope(main->parent);
    else
        parser_print_scope(main);
}

static void print_code(const struct code_bytecode *code, bool print_builtin)
{
    print_header("bytecode");
    code_print_bytecode(code, print_builtin);
}

static void print_stackmap(const struct code_bytecode *code)
{
    print_header("stackmap");
    code_print_stackmap(&code->stackmap);
}

static value_int_t exec_code(const struct code_bytecode *code, const struct interpreter_args *args,
        bool print_stack)
{
    struct vm_cpu vm = {{0}};
    struct vm_args vargs = {0};
    vargs.values = args->values;
    vargs.count = args->count;

    vm_enable_print_stack(&vm, print_stack);
    vm_execute_bytecode(&vm, code, &vargs);

    value_int_t ret = vm_get_stack_top(&vm);
    vm_cpu_clear(&vm);

    return ret;
}

value_int_t interpret_source(const char *text, const struct interpreter_args *args,
        const struct interpreter_option *opt)
{
    /* compile context */
    struct compile_context ctx;
    compile_context_init(&ctx);

    /* string intern */
    data_intern_table_init();

    /* type pool */
    parser_type_pool_init();

    /* exec passes */
    struct exec_pass pass = make_exec_pass(opt);

    /* builtin functions */
    struct parser_scope *builtin = parser_new_scope(NULL);
    define_builtin_functions(builtin);

    /* builtin modules */
    struct builtin_module_list builtin_modules = {0};
    builtin_register_modules(&builtin_modules);

    /* search paths */
    char *script_dir = get_script_dir(args->filename);
    struct parser_search_path paths = {0};

    parser_search_path_init(&paths, script_dir);
    /* TODO consdier passing builtin modules to parser_parse() separately
     * instead of holding them in struct parser_search_path */
    parser_search_path_add_builtin_modules(&paths, &builtin_modules);

    /* compile source code */
    struct parser_token *tok = NULL;
    struct parser_module *mod_main = NULL;
    value_int_t ret_code = 0;
    bool parse_error = false;

    if (setjmp(parse_env) == 0) {
        /* tokenize */
        if (pass.tokenize) {
            tok = parser_tokenize(text, args->filename, &ctx.token_pool);
        }

        /* print tokens */
        if (opt->print_token) {
            parser_print_token(tok, !opt->print_token_raw);
        }

        /* parse tokens */
        if (pass.parse) {
            struct parser_source source = {0};
            parser_source_init(&source, text, args->filename, "_main");
            mod_main = parser_parse(tok, builtin, &source, &paths, &ctx);
            code_resolve_offset(mod_main);
        }
    }
    else {
        parse_error = true;
        ret_code = EXIT_FAILURE;
    }

    /* generate and execute bytecode */
    struct code_bytecode code;
    code_bytecode_init(&code);

    if (!parse_error) {
        /* print tree */
        if (opt->print_tree) {
            print_tree(mod_main);
        }

        /* generate bytecode */
        code_bytecode_init(&code);
        if (pass.generate) {
            code_generate(&code, mod_main);
        }

        /* print symbols */
        if (opt->print_symbols) {
            print_syms(mod_main->scope, opt->print_symbols_all);
        }

        /* print bytecode */
        if (opt->print_bytecode) {
            print_code(&code, opt->print_bytecode_all);
        }

        /* print stackmap */
        if (opt->print_stackmap) {
            print_stackmap(&code);
        }

        /* execute */
        if (pass.execute) {
            ret_code = exec_code(&code, args, opt->print_stack);
        }
    }

    /* clean */
    code_bytecode_clear(&code);
    parser_free_scope(builtin);

    builtin_free_modules(&builtin_modules);
    parser_search_path_free(&paths);
    free(script_dir);

    parser_type_pool_free();
    data_intern_table_free();

    compile_context_clear(&ctx);

    return ret_code;
}
