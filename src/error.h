#ifndef ERROR_H
#define ERROR_H

#include <string>
#include "token.h"
#include "compiler.h"

void InternalError(const std::string &msg, const std::string &filename, int line);
void Error(const std::string &msg, const std::string &src, Pos pos);

#define ERROR_NO_CASE(e) (InternalError("No case found: " + \
            std::to_string(static_cast<int>(e)),__FILE__,__LINE__))

#endif // _H
