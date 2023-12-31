#include "interpreter.h"
#include "builtin.h"
#include "lexer.h"
#include "ast.h"
#include <iostream>
#include <iomanip>

static void print_header(const char *title)
{
    std::cout << "## " << title << std::endl;
    std::cout << "---" << std::endl;
}

Interpreter::Interpreter()
{
}

Interpreter::~Interpreter()
{
    delete prog_;
}

Int Interpreter::Run(const std::string &src)
{
    // Builtin functions
    DefineBuiltinFuncs(&scope_);

    // Print token
    if (print_token_) {
        print_token(src);
        if (!print_tree_ && !print_symbols_ && !print_bytecode_) {
            return 0;
        }
    }

    // Compile source
    prog_ = parser_.Parse(src, &scope_);

    if (print_tree_) {
        print_header("tree");
        prog_->Print();
    }

    if (print_symbols_) {
        print_header("symbol");
        if (print_symbols_all_)
            scope_.Print();
        else
            prog_->scope->Print();
    }

    // Generate bytecode
    SetOptimize(enable_optimize_);
    GenerateCode(prog_, code_);

    if (print_bytecode_) {
        print_header("bytecode");
        code_.Print();
    }

    // Run bytecode
    long ret = 0;
    if (!print_tree_ && !print_symbols_ && !print_bytecode_) {
        vm_.EnablePrintStack(print_stack_);
        vm_.Run(code_);
        ret = vm_.StackTopInt();
    }

    return ret;
}

void Interpreter::EnablePrintToken(bool enable, bool raw)
{
    print_token_ = enable;
    print_token_raw_ = raw;
}

void Interpreter::EnablePrintTree(bool enable)
{
    print_tree_ = enable;
}

void Interpreter::EnablePrintSymbols(bool enable, bool all)
{
    print_symbols_ = enable;
    print_symbols_all_ = all;
}

void Interpreter::EnablePrintBytecode(bool enable)
{
    print_bytecode_ = enable;
}

void Interpreter::EnablePrintStack(bool enable)
{
    print_stack_ = enable;
}

void Interpreter::EnableOptimize(bool enable)
{
    enable_optimize_ = enable;
}

void Interpreter::print_token(const std::string &src) const
{
    Lexer lexer;

    lexer.SetInput(src);
    Token token;
    Token *tok = &token;
    int indent = 0;
    bool bol = true;

    print_header("token");

    for (;;) {
        lexer.Get(tok);

        if (print_token_raw_) {
            std::cout << "(" <<
                std::setw(4) << tok->pos.y << ", " <<
                std::setw(3) << tok->pos.x << ") ";
            std::cout << tok->kind;

            if (tok->kind == TK::IDENT)
                std::cout << " (" << tok->sval << ")";
            if (tok->kind == TK::INTLIT)
                std::cout << " (" << tok->ival << ")";
            if (tok->kind == TK::FLTLIT)
                std::cout << " (" << tok->fval << ")";
            if (tok->kind == TK::STRLIT)
                std::cout << " (\"" << tok->sval << "\")";

            std::cout << std::endl;
        }
        else {
            if (tok->kind == TK::BLOCKBEGIN) {
                indent++;
                continue;
            }
            else if (tok->kind == TK::BLOCKEND) {
                indent--;
                continue;
            }

            if (bol) {
                bol = false;
                for (int i = 0; i < indent; i++)
                    std::cout << "....";
            }

            if (tok->kind == TK::NEWLINE) {
                std::cout << tok->kind << std::endl;
                bol = true;
            }
            else if (tok->kind != TK::BLOCKBEGIN && tok->kind != TK::BLOCKEND) {
                std::cout << tok->kind << ' ';
            }
        }

        if (tok->kind == TK::EOF_)
            break;
    }
    std::cout << std::endl;
}
