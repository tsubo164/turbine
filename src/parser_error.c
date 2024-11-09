#include "parser_error.h"
#include <stdlib.h>
#include <stdio.h>

static void print_detail(const char *src, int posx, int posy)
{
    int x = 0;
    int y = 1;
    const char *p = src;

    while (y != posy) {
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

    for (int i = 0; i < posx - 1; i++)
        printf(" ");

    printf("^\n");
}

void parser_error_va(const char *src, const char *filename,
        int posx, int posy, const char *fmt, va_list args)
{
    fprintf(stderr, "%s:%d:%d: error: ", filename, posx, posy);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    print_detail(src, posx, posy);

    exit(EXIT_FAILURE);
}
