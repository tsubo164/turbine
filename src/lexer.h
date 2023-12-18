#ifndef LEXER_H
#define LEXER_H

#include <string_view>
#include <iostream>
#include <string>
#include <stack>

#define TOKEN_LIST \
    /* TOKEN        STRING        */\
    /* ========================== */\
    TK(UNKNOWN,     "unknown") \
    /* factor */\
    TK(INTLIT,      "integer_literal") \
    TK(FLTLIT,      "float_literal") \
    TK(STRLIT,      "string_literal") \
    TK(IDENT,       "identifier") \
    /* operator */\
    TK(EQ,          "=") \
    TK(PLUSEQ,      "+=") \
    TK(MINUSEQ,     "-=") \
    TK(STAREQ,      "*=") \
    TK(SLASHEQ,     "/=") \
    TK(PERCENTEQ,   "%=") \
    TK(EQ2,         "==") \
    TK(EXCLEQ,      "!=") \
    TK(EXCL,        "!") \
    TK(CARET,       "^") \
    TK(TILDA,       "~") \
    TK(LT2,         "<<") \
    TK(GT2,         ">>") \
    TK(LT,          "<") \
    TK(GT,          ">") \
    TK(LTE,         "<=") \
    TK(GTE,         ">=") \
    TK(PLUS,        "+") \
    TK(MINUS,       "-") \
    TK(STAR,        "*") \
    TK(SLASH,       "/") \
    TK(PERCENT,     "%") \
    TK(BAR,         "|") \
    TK(BAR2,        "||") \
    TK(AMP,         "&") \
    TK(AMP2,        "&&") \
    TK(PERIOD,      ".") \
    TK(PLUS2,       "++") \
    TK(MINUS2,      "--") \
    TK(HASH,        "#") \
    TK(HASH2,       "##") \
    /* keyword */\
    TK(INT,         "int") \
    TK(FLOAT,       "float") \
    TK(STRING,      "string") \
    TK(IF,          "if") \
    TK(ELSE,        "else") \
    TK(FOR,         "for") \
    TK(BREAK,       "break") \
    TK(CONTINUE,    "continue") \
    TK(SWITCH,      "switch") \
    TK(CASE,        "case") \
    TK(DEFAULT,     "default") \
    TK(RETURN,      "return") \
    /* separator */\
    TK(COMMA,       ",") \
    TK(SEMICOLON,   ";") \
    TK(LPAREN,      "(") \
    TK(RPAREN,      ")") \
    TK(BLOCKBEGIN,  "block_begin") \
    TK(BLOCKEND,    "block_end") \
    TK(NEWLINE,     "\\n") \
    TK(EOF_,        "end_of_file")

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
    TokenKind kind = TK::UNKNOWN;
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
