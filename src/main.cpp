#include <string_view>
#include <iostream>
#include <fstream>
#include <string>
#include "interpreter.h"

std::string read_file(const std::string_view filename)
{
    std::ifstream ifs(filename);
    if (!ifs)
        return "";

    std::string text;
    std::string line;

    while (getline(ifs, line))
        text += line + '\n';

    return text;
}

int main(int argc, char **argv)
{
    bool print_token = false;
    bool print_token_raw = false;
    bool print_tree = false;
    bool print_symbols = false;
    bool print_bytecode = false;
    bool print_stack = false;
    std::string_view filename;

    for (int i = 1; i < argc; i++) {
        const std::string_view arg(argv[i]);

        if (arg == "--print-token" || arg == "-k") {
            print_token = true;
        }
        else if (arg == "--print-token-raw" || arg == "-K") {
            print_token = true;
            print_token_raw = true;
        }
        else if (arg == "--print-tree" || arg == "-t") {
            print_tree = true;
        }
        else if (arg == "--print-symbols" || arg == "-y") {
            print_symbols = true;
        }
        else if (arg == "--print-bytecode" || arg == "-b") {
            print_bytecode = true;
        }
        else if (arg == "--print-stack" || arg == "-s") {
            print_stack = true;
        }
        else if (arg[0] == '-') {
            std::cerr << "error: unknown option: " << arg << std::endl;
            std::exit(EXIT_FAILURE);
        }
        else {
            filename = arg;

            if (i != argc - 1) {
                std::cerr << "error: unknown argument after filename" << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }
    }

    const std::string src = read_file(filename);

    if (src.empty()) {
        std::cerr << "error: no such file: " << filename << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Interpreter ip;
    ip.EnablePrintToken(print_token, print_token_raw);
    ip.EnablePrintTree(print_tree);
    ip.EnablePrintSymbols(print_symbols);
    ip.EnablePrintBytecode(print_bytecode);
    ip.EnablePrintStack(print_stack);

    const int ret = ip.Run(src);
    if (!print_token && !print_tree && !print_bytecode && !print_symbols)
        std::cout << "ret: " << ret << std::endl;

    return 0;
}
