#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>

enum TokenKind {
    TOK_UNKNOWN = -1,
    TOK_INTNUM,
    TOK_EOF
};

struct Token {
    int kind = 0;
    long ival;
};

class Tokenizer {
public:
    Tokenizer();
    ~Tokenizer();

    void SetInput(std::istream &sstrm);
    int Get(Token &tok);

private:
    std::istream *strm_ = nullptr;

    int scan_number(int first_char, Token &tok);
};

#endif // _H
