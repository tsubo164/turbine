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

void Error(const std::string &msg, const std::string &src, Pos pos)
{
    const char *filename = "input.md";
    std::cerr << filename << ":" << pos.y << ":" << pos.x <<
        ": error: " << msg << std::endl;

    int x = 0;
    int y = 1;
    auto it = src.begin();

    while (y != pos.y) {
        if (*it++ == '\n') {
            x = 1;
            y++;
        }
        else {
            x++;
        }
    }

    while (*it != '\n') {
        std::cout << *it++;
    }
    std::cout << std::endl;

    std::cout << std::string(pos.x, ' ') << '^' << std::endl;
    exit(EXIT_FAILURE);
}
