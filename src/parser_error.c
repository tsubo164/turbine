#include "parser_error.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


jmp_buf parse_env;


static void print_detail(const char *srctext, int posx, int posy)
{
    assert(posx > 0 && posy > 0);

    int y = 1;
    const char *p = srctext;

    while (y != posy) {
        if (*p++ == '\n') {
            y++;
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

void parser_error_va(const char *srctext, const char *filename,
        int posx, int posy, const char *fmt, va_list args)
{
    fprintf(stderr, "%s:%d:%d: error: ", filename, posy, posx);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    print_detail(srctext, posx, posy);

    longjmp(parse_env, 1);
}
