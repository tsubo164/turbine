#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

void InternalError(const char *filename, int line, const char *fmt, ...);

#define UNREACHABLE (InternalError(__FILE__,__LINE__,"unreachable"))


#endif // _H
