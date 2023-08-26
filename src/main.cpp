#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "interpreter.h"

int main(int argc, char **argv)
{
    bool print_token = false;
    bool print_tree = false;
    bool print_symbols = false;
    bool print_bytecode = false;
    bool print_stack = false;
    std::string filename = "";

    for (int i = 1; i < argc; i++) {
        const std::string arg(argv[i]);

        if (arg == "--print-token" || arg == "-k") {
            print_token = true;
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

    std::ifstream stream(filename);

    if (!stream) {
        std::cerr << "error: no such file: " << filename << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Interpreter ip;
    ip.EnablePrintToken(print_token);
    ip.EnablePrintTree(print_tree);
    ip.EnablePrintSymbols(print_symbols);
    ip.EnablePrintBytecode(print_bytecode);
    ip.EnablePrintStack(print_stack);

    const int ret = ip.Run(stream);
    if (!print_token && !print_tree && !print_bytecode && !print_symbols)
        std::cout << "ret: " << ret << std::endl;

    return 0;
}
