#ifndef PARSER_ERROR_H
#define PARSER_ERROR_H

#include <stdarg.h>

void parser_error_va(const char *srctext, const char *filename,
        int posx, int posy, const char *fmt, va_list args);

#endif /* _H */
