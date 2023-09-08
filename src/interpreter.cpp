#include "interpreter.h"
#include "lexer.h"
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
    delete tree_;
}

Int Interpreter::Run(const std::string &src)
{
    // Print token
    if (print_token_) {
        return print_token(src);
    }

    // Compile source
    tree_ = parser_.Parse(src);

    if (print_tree_) {
        print_header("tree");
        tree_->Print();
    }

    if (print_symbols_) {
        print_header("symbol");
        scope_.Print();
    }

    // Generate bytecode
    GenerateCode(tree_, code_);

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

void Interpreter::EnablePrintSymbols(bool enable)
{
    print_symbols_ = enable;
}

void Interpreter::EnablePrintBytecode(bool enable)
{
    print_bytecode_ = enable;
}

void Interpreter::EnablePrintStack(bool enable)
{
    print_stack_ = enable;
}

int Interpreter::print_token(const std::string &src) const
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
            if (tok->kind == TK::Ident)
                std::cout << " (" << tok->sval << ")";
            std::cout << std::endl;
        }
        else {
            if (tok->kind == TK::BlockBegin) {
                indent++;
                continue;
            }
            else if (tok->kind == TK::BlockEnd) {
                indent--;
                continue;
            }

            if (bol) {
                bol = false;
                for (int i = 0; i < indent; i++)
                    std::cout << "....";
            }

            if (tok->kind == TK::NewLine) {
                std::cout << tok->kind << std::endl;
                bol = true;
            }
            else if (tok->kind != TK::BlockBegin && tok->kind != TK::BlockEnd) {
                std::cout << tok->kind << ' ';
            }
        }

        if (tok->kind == TK::Eof)
            break;
    }
    std::cout << std::endl;

    return 0;
}
