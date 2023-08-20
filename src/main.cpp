#include <iostream>
#include <sstream>
#include "interpreter.h"

int main()
{
    std::string src = "# main\n  id = 114 \n  id + 11\n  return\n  42 == 42\n  return 13";

    std::cout << "==========================" <<std::endl;
    std::cout << "\"" << src << "\"" << std::endl;
    std::cout << "==========================" <<std::endl;
    std::stringstream input(src);
    Interpreter ip;

    ip.EnablePrintTree(true);

    std::cout << "ret: " << ip.Run(input) << std::endl;

    return 0;
}
