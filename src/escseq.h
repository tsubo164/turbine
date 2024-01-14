#ifndef ESCSEQ_H
#define ESCSEQ_H

bool FindEscapedChar(int second_char, int *result_char);
int ConvertEscapeSequence(const char *src, const char **dst);

#endif // _H
