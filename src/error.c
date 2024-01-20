#include "error.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

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

void InternalError(const char *filename, int line, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%s:%d: internal error: ", filename, line);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);

    exit(EXIT_FAILURE);
}
