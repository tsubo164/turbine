#include "escseq.h"

void ConvertEscapeSequence(std::string_view src, std::string &dst)
{
    dst.reserve(src.length());

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

    for (int i = 0; i < src.length(); i++) {
        int ch = src[i];

        if (ch == '\\') {
            const int next = src[i + 1];

            for (int j = 0; j < N; j++) {
                if (next == table[j][0]) {
                    ch = table[j][1];
                    i++;
                    break;
                }
            }
        }
        dst += ch;
    }
}
