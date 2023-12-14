#ifndef LEXER_H
#define LEXER_H

#include <string_view>
#include <iostream>
#include <string>
#include <stack>

#define TOKEN_LIST \
    /* TOKEN       TOKEN_STRING   */\
    /* ========================== */\
    TK(Unknown,     "Unknown") \
    /* factor */\
    TK(IntNum,      "IntNum") \
    TK(FpNum,       "FpNum") \
    TK(StringLit,   "StringLit") \
    TK(Ident,       "Ident") \
    /* operator */\
    TK(Equal,       "=") \
    TK(Equal2,      "==") \
    TK(ExclEqual,   "!=") \
    TK(EXCL,        "!") \
    TK(LT,          "<") \
    TK(GT,          ">") \
    TK(LTE,         "<=") \
    TK(GTE,         ">=") \
    TK(Plus,        "+") \
    TK(Minus,       "-") \
    TK(STAR,        "*") \
    TK(Slash,       "/") \
    TK(PERCENT,     "%") \
    TK(BAR,         "|") \
    TK(BAR2,        "||") \
    TK(AMP,         "&") \
    TK(AMP2,        "&&") \
    TK(Period,      ".") \
    TK(PLUS2,       "++") \
    TK(MINUS2,      "--") \
    TK(Hash,        "#") \
    TK(Hash2,       "##") \
    /* keyword */\
    TK(Int,         "int") \
    TK(Float,       "float") \
    TK(String,      "string") \
    TK(If,          "if") \
    TK(Else,        "else") \
    TK(Return,      "return") \
    /* separator */\
    TK(Comma,       ",") \
    TK(LParen,      "(") \
    TK(RParen,      ")") \
    TK(BlockBegin,  "BlockBegin") \
    TK(BlockEnd,    "BlockEnd") \
    TK(NewLine,     "\\n") \
    TK(Eof,         "EOF")

enum class TokenKind {
#define TK(tok, str) tok,
    TOKEN_LIST
#undef TK
};
using TK = TokenKind;

const char *GetTokenKindString(TokenKind kind);
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
