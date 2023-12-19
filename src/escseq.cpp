#include "escseq.h"

bool FindEscapedChar(int second_char, int &result_char)
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
            result_char = table[j][1];
            return true;
        }
    }
    return false;
}

int ConvertEscapeSequence(std::string_view src, std::string &dst)
{
    dst.reserve(src.length());

    for (int i = 0; i < src.length(); i++) {
        int ch = src[i];

        if (ch == '\\') {
            const int next = src[i + 1];
            const bool found = FindEscapedChar(next, ch);

            if (found) {
                i++;
            }
            else {
                // error
                return i;
            }
        }
        dst += ch;
    }
    return -1;
}
