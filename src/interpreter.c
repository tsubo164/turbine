#include "interpreter.h"
#include "builtin.h"
#include "codegen.h"
#include "parser.h"
#include "token.h"
#include "scope.h"
#include "ast.h"
#include "vm.h"

#include <stdio.h>

static void print_header(const char *title)
{
    printf("## %s\n", title);
    printf("---\n");
}

Int Interpret(const char *src, const Option *opt)
{
    const Token *tok = NULL;
    Scope builtin = {0};
    Scope *global = NULL;
    Prog prog = {0};
    Bytecode code = {{0}};
    VM vm = {{0}};

    // Builtin functions
    DefineBuiltinFuncs(&builtin);

    // Tokenize
    tok = Tokenize(src);

    // Print token
    if (opt->print_token) {
        PrintToken(tok, !opt->print_token_raw);
        if (!opt->print_tree && !opt->print_symbols && !opt->print_bytecode) {
            return 0;
        }
    }

    // Compile source
    global = OpenChild(&builtin);
    prog.scope = global;
    Parse(src, tok, global, &prog);

    if (opt->print_tree) {
        print_header("tree");
        PrintProg(&prog, 0);
    }

    if (opt->print_symbols) {
        print_header("symbol");
        if (opt->print_symbols_all)
            PrintScope(&builtin, 0);
        else
            PrintScope(global, 0);
    }

    // Generate bytecode
    SetOptimize(opt->enable_optimize);
    GenerateCode(&code, &prog);

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

