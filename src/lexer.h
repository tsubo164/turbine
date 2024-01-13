#ifndef LEXER_H
#define LEXER_H

#include <string_view>
#include <iostream>
#include <string>
#include <stack>

enum class TokenKind {
    UNKNOWN = 0,
    // factor
    INTLIT,
    FLTLIT,
    STRLIT,
    IDENT,
    // operator
    EQ,
    PLUSEQ,
    MINUSEQ,
    STAREQ,
    SLASHEQ,
    PERCENTEQ,
    EQ2,
    EXCLEQ,
    EXCL,
    CARET,
    TILDA,
    LT2,
    GT2,
    LT,
    GT,
    LTE,
    GTE,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    BAR,
    BAR2,
    AMP,
    AMP2,
    PERIOD,
    PLUS2,
    MINUS2,
    HASH,
    HASH2,
    // keyword
    NIL,
    TRUE,
    FALSE,
    BOOL,
    INT,
    FLOAT,
    STRING,
    IF,
    OR,
    ELSE,
    FOR,
    BREAK,
    CONTINUE,
    SWITCH,
    CASE,
    DEFAULT,
    RETURN,
    NOP,
    // separator
    MINUS3,
    COMMA,
    SEMICOLON,
    LPAREN,
    RPAREN,
    LBRACK,
    RBRACK,
    BLOCKBEGIN,
    BLOCKEND,
    NEWLINE,
    // special var
    CALLER_LINE,
    EOF_,
};
using TK = TokenKind;

const char *GetTokenKindString(TokenKind kind);
std::ostream &operator<<(std::ostream &os, TokenKind kind);

struct Pos {
    int x = 0, y = 1;
};

struct Token {
    TokenKind kind = TK::UNKNOWN;
    Pos pos;

    long ival = 0;
    double fval = 0.0;
    bool has_escseq = false;
    std::string_view sval;
    const char *sval_;

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
    void scan_char_literal(Token *tok, Pos pos);
    void scan_word(Token *tok, Pos pos);
    void scan_string(Token *tok, Pos pos);
    int count_indent();
    TokenKind scan_indent(Token *tok);
    void scan_line_comment();
    void scan_block_comment(Pos pos);
};

#endif // _H
