#ifndef PARSER_ESCSEQ_H
#define PARSER_ESCSEQ_H

#include <stdbool.h>

bool parser_find_escaped_char(int second_char, int *result_char);
int parser_convert_escape_sequence(const char *src, const char **dst);

#endif /* _H */
