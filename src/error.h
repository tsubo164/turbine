#ifndef ERROR_H
#define ERROR_H

#include "token.h"
#include <stdarg.h>

void Error(const char *src, const char *filename, struct Pos pos, const char *fmt, ...);
void VError(const char *src, const char *filename, struct Pos pos, const char *fmt, va_list args);
void InternalError(const char *filename, int line, const char *fmt, ...);

#define UNREACHABLE (InternalError(__FILE__,__LINE__,"unreachable"))


#endif // _H
