#ifndef ERROR_H
#define ERROR_H

#include <string>

void InternalError(const std::string &msg, const std::string &filename, int line);

#define ERROR_NO_CASE(e) (InternalError("No case found: " + \
            std::to_string(static_cast<int>(e)),__FILE__,__LINE__))

#endif // _H
