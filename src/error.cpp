#include "error.h"
#include <iostream>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

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

    std::cout << std::string(pos.x - 1, ' ') << '^' << std::endl;
    exit(EXIT_FAILURE);
}

static void print_detail(const char *src, Pos pos)
{
    int x = 0;
    int y = 1;
    const char *p = src;

    while (y != pos.y) {
        if (*p++ == '\n') {
            x = 1;
            y++;
        }
        else {
            x++;
        }
    }

    while (*p != '\n') {
        printf("%c", *p++);
    }
    printf("\n");

    for (int i = 0; i < pos.x - 1; i++)
        printf(" ");
    printf("^\n");
}

void Error(const char *src, const char *filename, Pos pos, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    VError(src, filename, pos, fmt, args);
    va_end(args);
}

void VError(const char *src, const char *filename, Pos pos, const char *fmt, va_list args)
{
    fprintf(stderr, "%s:%d:%d: error: ", filename, pos.x, pos.y);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    print_detail(src, pos);

    exit(EXIT_FAILURE);
}
