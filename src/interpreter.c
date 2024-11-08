#include "interpreter.h"
#include "builtin.h"
#include "codegen.h"
#include "data_intern.h"
#include "parser.h"
#include "parser_token.h"
#include "parser_print.h"
#include "parser_ast.h"
#include "scope.h"
#include "vm.h"

#include <stdio.h>

static void print_header(const char *title)
{
    printf("## %s\n", title);
    printf("---\n");
}

Int Interpret(const char *src, const char *filename, const Option *opt)
{
    const struct parser_token *tok = NULL;
    struct Scope builtin = {0};
    Bytecode code = {{0}};
    VM vm = {{0}};

    // Builtin functions
    DefineBuiltinFuncs(&builtin);

    // Tokenize
    tok = parser_tokenize(src);

    // Print token
    if (opt->print_token) {
        parser_print_token(tok, !opt->print_token_raw);
        if (!opt->print_tree && !opt->print_symbols && !opt->print_bytecode) {
            return 0;
        }
    }

    // Compile source
    struct Module *prog = Parse(src, filename, data_string_intern("_main"), tok, &builtin);
    ResolveOffset(prog);

    if (opt->print_tree) {
        print_header("tree");
        parser_print_prog(prog);
    }

    if (opt->print_symbols) {
        print_header("symbol");
        if (opt->print_symbols_all)
            parser_print_scope(&builtin);
        else
            parser_print_scope(prog->scope);
    }

    // Generate bytecode
    SetOptimize(opt->enable_optimize);
    GenerateCode(&code, prog);

    if (opt->print_bytecode) {
        print_header("bytecode");
        PrintBytecode(&code);
    }

    // Run bytecode
    long ret = 0;
    if (!opt->print_tree && !opt->print_symbols && !opt->print_bytecode) {
        EnablePrintStack(&vm, opt->print_stack);
        Run(&vm, &code);
        ret = StackTopInt(&vm);
    }

    return ret;
}
