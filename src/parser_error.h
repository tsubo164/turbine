#ifndef PARSER_ERROR_H
#define PARSER_ERROR_H

#include <stdarg.h>
#include <setjmp.h>

extern jmp_buf parse_env;

void parser_error_va(const char *srctext, const char *filename,
        int posx, int posy, const char *fmt, va_list args);

#endif /* _H */
