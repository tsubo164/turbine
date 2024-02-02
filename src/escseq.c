#include "escseq.h"
#include "intern.h"

bool FindEscapedChar(int second_char, int *result_char)
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
    static const int N = sizeof(table) / sizeof(table[0]);

    for (int j = 0; j < N; j++) {
        if (second_char == table[j][0]) {
            *result_char = table[j][1];
            return true;
        }
    }
    return false;
}

int ConvertEscapeSequence(const char *src, const char **dst)
{
    static char buf[4096] = {'\0'};
    const char *s = src;
    char *d = buf;
    int i = 0;

    while (*s) {
        int ch = *s;

        if (ch == '\\') {
            const int next = *++s;
            const bool found = FindEscapedChar(next, &ch);

            if (found) {
                i++;
            }
            else {
                // error
                *dst = StrIntern(buf);
                return i;
            }
        }
        *d++ = ch;
        s++;
    }
    return -1;
}
