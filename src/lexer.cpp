#include "lexer.h"
#include "error.h"
#include "escseq.h"
#include <unordered_map>
#include <cstdlib>
#include <cassert>

static const std::unordered_map<std::string_view, TokenKind> keywords = {
    {"nil",      TK::NIL},
    {"true",     TK::TRUE},
    {"false",    TK::FALSE},
    {"int",      TK::INT},
    {"float",    TK::FLOAT},
    {"string",   TK::STRING},
    {"if",       TK::IF},
    {"else",     TK::ELSE},
    {"for",      TK::FOR},
    {"break",    TK::BREAK},
    {"continue", TK::CONTINUE},
    {"switch",   TK::SWITCH},
    {"case",     TK::CASE},
    {"default",  TK::DEFAULT},
    {"return",   TK::RETURN},
    {"nop",      TK::NOP},
};

static TokenKind keyword_or_identifier(std::string_view word)
{
    const auto it = keywords.find(word);

    if (it != keywords.end())
        return it->second;

    return TK::IDENT;
}

static const char *tok_kind_string(TokenKind kind)
{
    switch (kind) {
#define TK(tok, str) case TokenKind::tok: return str;
    TOKEN_LIST
#undef TK
    default:
        ERROR_NO_CASE(kind);
        return nullptr;
    }
}

const char *GetTokenKindString(TokenKind kind)
{
    return tok_kind_string(kind);
}

std::ostream &operator<<(std::ostream &os, TokenKind kind)
{
    return os << tok_kind_string(kind);
}

void Token::set(TokenKind k, Pos p)
{
    kind = k;
    pos = p;
}

Lexer::Lexer()
{
    indent_stack_.push(0);
    is_line_begin_ = true;
}

Lexer::~Lexer()
{
}

void Lexer::SetInput(const std::string &src)
{
    src_ = &src;
    it_ = src_->begin();
    pos_ = {};
}

int Lexer::get()
{
    prevx = pos_.x;

    if (curr() == '\n') {
        pos_.x = 1;
        pos_.y++;
    }
    else {
        pos_.x++;
    }

    return *it_++;
}

int Lexer::peek()
{
    return *it_;
}

void Lexer::unget()
{
    it_--;

    if (curr() == '\n') {
        pos_.x = prevx;
        prevx--;
        pos_.y--;
    }
    else {
        pos_.x--;
    }
}

bool Lexer::eof() const
{
    return it_ == src_->end();
}

int Lexer::curr() const
{
    if (it_ == src_->begin())
        return '\0';
    else
        return *(it_ - 1);
}

void Lexer::Get(Token *tok)
{
    *tok = {};

    if (unread_blockend_ > 0) {
        unread_blockend_--;
        tok->set(TK::BLOCKEND, pos_);
        return;
    }

    if (is_line_begin_) {
        is_line_begin_ = false;

        const TokenKind kind = scan_indent(tok);
        if (kind == TK::BLOCKBEGIN || kind == TK::BLOCKEND)
            return;
    }

    while (!eof()) {
        int ch = get();
        const Pos pos = pos_;

        // number
        if (isdigit(ch)) {
            unget();
            scan_number(tok, pos);
            return;
        }

        if (ch == '\'') {
            scan_char_literal(tok, pos);
            return;
        }

        if (ch == '=') {
            ch = get();

            if (ch == '=') {
                tok->set(TK::EQ2, pos);
            }
            else {
                unget();
                tok->set(TK::EQ, pos);
            }
            return;
        }

        if (ch == '!') {
            ch = get();

            if (ch == '=') {
                tok->set(TK::EXCLEQ, pos);
            }
            else {
                unget();
                tok->set(TK::EXCL, pos);
            }
            return;
        }

        if (ch == '^') {
            tok->set(TK::CARET, pos);
            return;
        }

        if (ch == '~') {
            tok->set(TK::TILDA, pos);
            return;
        }

        if (ch == '<') {
            ch = get();
            if (ch == '<') {
                tok->set(TK::LT2, pos);
            }
            else if (ch == '=') {
                tok->set(TK::LTE, pos);
            }
            else {
                unget();
                tok->set(TK::LT, pos);
            }
            return;
        }

        if (ch == '>') {
            ch = get();
            if (ch == '>') {
                tok->set(TK::GT2, pos);
            }
            else if (ch == '=') {
                tok->set(TK::GTE, pos);
            }
            else {
                unget();
                tok->set(TK::GT, pos);
            }
            return;
        }

        if (ch == '+') {
            ch = get();
            if (ch == '+') {
                tok->set(TK::PLUS2, pos);
            }
            else if (ch == '=') {
                tok->set(TK::PLUSEQ, pos);
            }
            else {
                unget();
                tok->set(TK::PLUS, pos);
            }
            return;
        }

        if (ch == '-') {
            ch = get();
            if (ch == '-') {
                tok->set(TK::MINUS2, pos);
            }
            else if (ch == '=') {
                tok->set(TK::MINUSEQ, pos);
            }
            else {
                unget();
                tok->set(TK::MINUS, pos);
            }
            return;
        }

        if (ch == '*') {
            ch = get();
            if (ch == '=') {
                tok->set(TK::STAREQ, pos);
            }
            else {
                unget();
                tok->set(TK::STAR, pos);
            }
            return;
        }

        if (ch == '/') {
            ch = get();
            if (ch == '/') {
                scan_line_comment();
                continue;
            }
            else if (ch == '*') {
                scan_block_comment(pos);
                continue;
            }
            else if (ch == '=') {
                tok->set(TK::SLASHEQ, pos);
            }
            else {
                unget();
                tok->set(TK::SLASH, pos);
            }
            return;
        }

        if (ch == '%') {
            ch = get();
            if (ch == '=') {
                tok->set(TK::PERCENTEQ, pos);
            }
            else {
                unget();
                tok->set(TK::PERCENT, pos);
            }
            return;
        }

        if (ch == '|') {
            ch = get();
            if (ch == '|') {
                tok->set(TK::BAR2, pos);
            }
            else {
                unget();
                tok->set(TK::BAR, pos);
            }
            return;
        }

        if (ch == '&') {
            ch = get();
            if (ch == '&') {
                tok->set(TK::AMP2, pos);
            }
            else {
                unget();
                tok->set(TK::AMP, pos);
            }
            return;
        }

        if (ch == '.') {
            tok->set(TK::PERIOD, pos);
            return;
        }

        if (ch == ',') {
            tok->set(TK::COMMA, pos);
            return;
        }

        if (ch == ';') {
            tok->set(TK::SEMICOLON, pos);
            return;
        }

        if (ch == '(') {
            tok->set(TK::LPAREN, pos);
            return;
        }

        if (ch == ')') {
            tok->set(TK::RPAREN, pos);
            return;
        }

        // word
        if (isalpha(ch)) {
            unget();
            scan_word(tok, pos);
            return;
        }

        // string
        if (ch == '"') {
            scan_string(tok, pos);
            return;
        }

        if (ch == '#') {
            ch = get();
            if (ch == '#') {
                tok->set(TK::HASH2, pos);
            }
            else {
                unget();
                tok->set(TK::HASH, pos);
            }
            return;
        }

        if (ch == '\n') {
            tok->set(TK::NEWLINE, pos);
            is_line_begin_ = true;
            return;
        }

        if (ch == EOF) {
            tok->set(TK::EOF_, pos);
            return;
        }

        // skip
        if (ch == ' ' || ch == '\t' || ch == '\v') {
            continue;
        }

        Error("unknown token", *src_, pos_);
        return;
    }

    tok->set(TK::EOF_, pos_);
}

static bool isfp(int ch)
{
    const int c = tolower(ch);

    return c == '.' || c == 'e';
}

static bool ishex(int ch)
{
    const int c = tolower(ch);

    return c == 'x' ||
        c == 'a' || c == 'b' || c == 'c' ||
        c == 'd' || c == 'e' || c == 'f';
}

static bool isnum(int ch)
{
    return isdigit(ch) || ishex(ch) || isfp(ch);
}

void Lexer::scan_number(Token *tok, Pos pos)
{
    auto start = it_;
    bool fpnum = false;
    int base = 10;
    int len = 0;

    for (int ch = get(); isnum(ch); ch = get()) {
        if (ishex(ch))
            base = 16;

        if (isfp(ch))
            fpnum = true;

        if (ch == 'e' || ch == 'E') {
            if (peek() == '-' || peek() == '+') {
                continue;
            }
            else {
                // reject 'e'/'E'
                unget();
                break;
            }
        }

        len++;
    }

    unget();

    char *end = nullptr;

    if (fpnum) {
        tok->fval = strtod(&(*start), &end);
        tok->set(TK::FLTLIT, pos);
    }
    else {
        tok->ival = strtol(&(*start), &end, base);
        tok->set(TK::INTLIT, pos);
    }

    assert(end && (len == (end - &(*start))));
}

void Lexer::scan_char_literal(Token *tok, Pos pos)
{
    int ch = get();

    if (ch == '\\') {
        const int next = get();
        const bool found = FindEscapedChar(next, ch);
        if (!found) {
            unget();
            Error("unknown escape sequence", *src_, pos_);
        }
    }

    tok->ival = ch;
    tok->set(TK::INTLIT, pos);

    ch = get();
    if (ch != '\'') {
        unget();
        Error("unterminated char literal", *src_, pos_);
    }
}

static bool isword(int ch)
{
    return isalnum(ch) || ch == '_';
}

void Lexer::scan_word(Token *tok, Pos pos)
{
    auto start = it_;
    int len = 0;

    for (int ch = get(); isword(ch); ch = get())
        len++;

    unget();

    const std::string_view word(&(*start), len);
    const TokenKind kind = keyword_or_identifier(word);

    if (kind == TK::IDENT)
        tok->sval = word;

    tok->set(kind, pos);
}

void Lexer::scan_string(Token *tok, Pos pos)
{
    const Pos strpos = pos;
    auto start = it_;
    int len = 0;
    int backslashes = 0;

    for (int ch = get(); ch != '"'; ch = get()) {
        const int next = peek();

        if (ch == '\\') {
            backslashes++;
            if (next == '"' || next == '\\') {
                ch = get();
                len++;
            }
        }

        if (ch == EOF || ch == '\0') {
            unget();
            Error("unterminated string literal", *src_, strpos);
        }

        len++;
    }

    const std::string_view str_lit(&(*start), len);

    tok->has_escseq = backslashes > 0;
    tok->sval = str_lit;
    tok->set(TK::STRLIT, pos);
}

int Lexer::count_indent()
{
    int indent = 0;

    for (;;) {
        const int ch = get();

        if (ch == '/') {
            // indent + line comment => next line
            if (peek() == '/') {
                scan_line_comment();
                continue;
            }
            else {
                unget();
                break;
            }
        }
        else if (ch == ' ' || ch == '\v' || ch == '\f') {
            indent++;
            continue;
        }
        else if (ch == '\t') {
            indent += 4;
            continue;
        }
        else if (ch == '\n') {
            // blank line => next line
            indent = 0;
            continue;
        }
        else {
            unget();
            break;
        }
    }

    return indent;
}

TokenKind Lexer::scan_indent(Token *tok)
{
    const int indent = count_indent();

    if (indent > indent_stack_.top()) {
        // push indent
        indent_stack_.push(indent);
        tok->set(TK::BLOCKBEGIN, pos_);

        // BlockBegin alwasy starts at beginning of line
        tok->pos.x = 1;

        return tok->kind;
    }
    else if (indent < indent_stack_.top()) {
        // pop indents until it matches current
        unread_blockend_ = 0;

        while (indent < indent_stack_.top()) {
            indent_stack_.pop();

            if (indent == indent_stack_.top()) {
                tok->set(TK::BLOCKEND, pos_);
                return tok->kind;
            }

            unread_blockend_++;
        }

        // no indent matches current
        Error("mismatch outer indent", *src_, pos_);
        return tok->kind;
    }
    else {
        // no indent change
        return tok->kind;
    }
}

void Lexer::scan_line_comment()
{
    for (;;) {
        const int ch = get();

        if (ch == '\n') {
            unget();
            break;
        }
    }
}

void Lexer::scan_block_comment(Pos pos)
{
    const Pos commentpos = pos;
    // already accepted "/*"
    int depth = 1;

    for (;;) {
        int ch = get();

        if (ch == '/') {
            ch = get();
            if (ch == '*') {
                depth++;
                continue;
            }
        }

        if (ch == '*') {
            ch = get();
            if (ch == '/') {
                depth--;
                if (depth == 0)
                    break;
                else
                    continue;
            }
        }

        if (ch == EOF || ch == '\0') {
            unget();
            Error("unterminated block comment", *src_, commentpos);
        }
    }
}
