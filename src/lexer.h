#ifndef LEXER_H
#define LEXER_H

#include <iostream>
#include <string>
#include <stack>
#include "string_table.h"

enum class TokenKind {
    Eof = -1,
    Unknown = 0,
    // factor
    IntNum,
    Ident,
    // operator
    Equal,
    Equal2,
    Plus,
    Minus,
    Slash,
    Period,
    Hash,
    Hash2,
    // keyword
    Int,
    If,
    Else,
    Return,
    String,
    // separator
    Comma,
    LParen,
    RParen,
    BlockBegin,
    BlockEnd,
    NewLine,
};
using TK = TokenKind;

std::ostream &operator<<(std::ostream &os, TokenKind kind);

struct Token {
    TokenKind kind = TK::Unknown;
    long ival = 0;
    const char *sval = nullptr;
};

class Lexer {
public:
    Lexer(StringTable &strtab);
    ~Lexer();

    void SetInput(std::istream &stream);
    void Get(Token *tok);

private:
    // stream
    std::istream *stream_ {};
    StringTable &strtable_;
    std::string strbuf_;

    // indent
    std::stack <int>indent_stack_;
    int unread_blockend_ = 0;
    bool is_line_begin_ = true;

    int get();
    int peek();
    void unget();
    bool eof() const;

    void scan_number(int first_char, Token *tok);
    void scan_word(int first_char, Token *tok);
    int count_indent();
    TokenKind scan_indent(Token *tok);
    void scan_line_comment();
};

#endif // _H
