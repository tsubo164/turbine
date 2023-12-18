#ifndef ESCSEQ_H
#define ESCSEQ_H

#include <string_view>
#include <string>

void ConvertEscapeSequence(std::string_view src, std::string &dst);

#endif // _H
