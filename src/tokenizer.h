#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>
#include <string>
#include <stack>
#include "string_table.h"

enum class TokenKind {
    Eof = -1,
    Unknown = 0,
    IntNum,
    Ident,

    Equal,
    Equal2,
    Plus,
    Minus,
    Hash,

    Int,
    If,
    Return,

    Comma,
    LeftParenthesis,
    RightParenthesis,
    BlockBegin,
    BlockEnd,
    NewLine,
};

using TK = enum TokenKind;

std::ostream &operator<<(std::ostream &os, TokenKind kind);

struct Token {
    TokenKind kind = TK::Unknown;
    long ival = 0;
    SharedStr sval {};
};

class Tokenizer {
public:
    Tokenizer(StringTable &string_table);
    ~Tokenizer();

    void SetInput(std::istream &stream);
    void Get(Token *tok);

private:
    std::istream *stream_ {};
    StringTable &strtable_;
    std::string strbuf_;

    std::stack <int>indent_stack_;
    int unread_blockend_ = 0;
    bool is_line_begin_ = true;

    TokenKind scan_number(int first_char, Token *tok);
    TokenKind scan_word(int first_char, Token *tok);
    int scan_indent();
};

#endif // _H
