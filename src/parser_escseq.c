#include "parser_escseq.h"
#include "data_intern.h"

bool parser_find_escaped_char(int second_char, int *result_char)
{
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

    for (int j = 0; j < N; j++) {
        if (second_char == table[j][0]) {
            *result_char = table[j][1];
            return true;
        }
    }

    return false;
}

int parser_convert_escape_sequence(const char *src, const char **dst)
{
    /* TODO take care of buffer overflow */
    static char buf[4096] = {'\0'};
    const char *s = src;
    char *d = buf;
    int i = 0;

    while (*s) {
        int ch = *s;

        if (ch == '\\') {
            int next = *++s;
            bool found = parser_find_escaped_char(next, &ch);

            if (found) {
                i++;
            }
            else {
                /* error */
                return i;
            }
        }
        *d++ = ch;
        s++;
    }

    *d = '\0';
    *dst = data_string_intern(buf);
    return -1;
}
