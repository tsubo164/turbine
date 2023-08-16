#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>
#include <string>
#include "string_table.h"

enum class TokenKind {
    Eof = -1,
    IntNum,
    Ident,
    If,
    Equal,
    Plus,
    Unknown,
};
using TK = enum TokenKind;

struct Token {
    TokenKind kind = TK::Unknown;
    long ival = 0;
    int str_id = 0;
};

class Tokenizer {
public:
    Tokenizer() {}
    ~Tokenizer() {}

    void SetInput(std::istream &stream);
    void SetStringTable(StringTable &str_table);
    TokenKind Get(Token &tok);

private:
    std::istream *stream_ = nullptr;
    StringTable *strtable_ = nullptr;
    std::string strbuf_;

    TokenKind scan_number(int first_char, Token &tok);
    TokenKind scan_word(int first_char, Token &tok);
};

#endif // _H
