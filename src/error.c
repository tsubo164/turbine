#include "error.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

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
