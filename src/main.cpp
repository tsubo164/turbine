#include <iostream>
#include <sstream>
#include "interpreter.h"

int main()
{
    std::string src = "  id = 114 \n  id + 11";

    std::cout << "==========================" <<std::endl;
    std::cout << "\"" << src << "\"" << std::endl;
    std::cout << "==========================" <<std::endl;
    std::stringstream input(src);
    Interpreter ip;

    ip.EnablePrintTree(true);

    ip.Run(input);

    return 0;
}
