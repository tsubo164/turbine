#ifndef LEXER_H
#define LEXER_H

#include <string_view>
#include <iostream>
#include <string>
#include <stack>

enum class TokenKind {
    Eof = -1,
    Unknown = 0,
    // factor
    IntNum,
    FpNum,
    StringLit,
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
    Float,
    String,
    If,
    Else,
    Return,
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

struct Pos {
    int x = 0, y = 1;
};

struct Token {
    TokenKind kind = TK::Unknown;
    Pos pos;

    long ival = 0;
    double fval = 0.0;
    std::string_view sval;

    void set(TokenKind k, Pos p);
};

class Lexer {
public:
    Lexer();
    ~Lexer();

    void SetInput(const std::string &src);
    void Get(Token *tok);

private:
    // src text
    const std::string *src_ {};
    std::string::const_iterator it_;
    Pos pos_;
    int prevx = 0;

    // indent
    std::stack <int>indent_stack_;
    int unread_blockend_ = 0;
    bool is_line_begin_ = true;

    int get();
    int peek();
    void unget();
    bool eof() const;
    int curr() const;

    void scan_number(Token *tok, Pos pos);
    void scan_word(Token *tok, Pos pos);
    void scan_string(Token *tok, Pos pos);
    int count_indent();
    TokenKind scan_indent(Token *tok);
    void scan_line_comment();
};

#endif // _H
