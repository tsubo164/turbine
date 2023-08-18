#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>
#include <string>
#include "string_table.h"

enum class TokenKind {
    Eof = -1,
    Unknown = 0,
    IntNum,
    Ident,
    If,
    Equal,
    Plus,
    NewLine,
};

using TK = enum TokenKind;

struct Token {
    TokenKind kind = TK::Unknown;
    long ival = 0;
    int str_id = 0;
};

class Tokenizer {
public:
    Tokenizer(StringTable &string_table) : strtable_(string_table) {}
    ~Tokenizer() {}

    void SetInput(std::istream &stream);
    void Get(Token *tok);

private:
    std::istream *stream_ = nullptr;
    StringTable &strtable_;
    std::string strbuf_;

    TokenKind scan_number(int first_char, Token *tok);
    TokenKind scan_word(int first_char, Token *tok);
};

#endif // _H
