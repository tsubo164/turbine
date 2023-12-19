#ifndef ESCSEQ_H
#define ESCSEQ_H

#include <string_view>
#include <string>

bool FindEscapedChar(int second_char, int &result_char);
int ConvertEscapeSequence(std::string_view src, std::string &dst);

#endif // _H
