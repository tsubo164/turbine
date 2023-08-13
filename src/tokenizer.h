#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>
#include <string>
#include "string_table.h"

enum TokenKind {
    TOK_EOF = -1,
    TOK_INTNUM,
    TOK_IDENT,
    TOK_IF,
    TOK_PLUS,
    TOK_UNKNOWN,
};

struct Token {
    int kind = 0;

    long ival = 0;
    int str_id = 0;
};

class Tokenizer {
public:
    Tokenizer();
    ~Tokenizer();

    void SetInput(std::istream &stream);
    void SetStringTable(StringTable &str_table);
    int Get(Token &tok);

private:
    std::istream *stream_ = nullptr;
    StringTable *strtable_ = nullptr;
    std::string strbuf_;

    int scan_number(int first_char, Token &tok);
    int scan_word(int first_char, Token &tok);
};

#endif // _H
