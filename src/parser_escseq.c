#include "parser_escseq.h"

static const int table[][2] = {
    {'"',  '"'},
    {'0',  '\0'},
    {'n',  '\n'},
    {'\'', '\''},
    {'\\', '\\'},
    {'t',  '\t'},
    {'r',  '\r'},
    {'f',  '\f'},
    {'v',  '\v'},
    {'a',  '\a'},
    {'b',  '\b'},
    {'?',  '\?'},
};
static int N = sizeof(table) / sizeof(table[0]);

int parser_find_escape_sequence(int escape_char)
{
    for (int i = 0; i < N; i++) {
        if (escape_char == table[i][0])
            return table[i][1];
    }
    return -1;
}
