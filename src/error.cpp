#include "error.h"
#include <iostream>
#include <cstdlib>

void InternalError(const std::string &msg, const std::string &filename, int line)
{
    std::cerr << "internal error: " <<
        filename << ":" <<
        line << ": " <<
        msg << std::endl;
    exit(EXIT_FAILURE);
}
