#ifndef ERROR_H
#define ERROR_H

#include <string>
#include "token.h"
#include "compiler.h"

void InternalError(const std::string &msg, const std::string &filename, int line);
void Error(const std::string &msg, const std::string &src, Pos pos);

#define ERROR_NO_CASE(e) (InternalError("No case found: " + \
            std::to_string(static_cast<int>(e)),__FILE__,__LINE__))

void Error(const char *src, const char *filename, Pos pos, const char *fmt, ...);
void VError(const char *src, const char *filename, Pos pos, const char *fmt, va_list args);

#endif // _H
