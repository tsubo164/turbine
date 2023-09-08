#include "lexer.h"
#include "error.h"
#include <unordered_map>
#include <cstdlib>
#include <cassert>

static const std::unordered_map<std::string_view, TokenKind> keywords = {
    {"int", TK::Int},
    {"if", TK::If},
    {"else", TK::Else},
    {"return", TK::Return},
    {"string", TK::String},
};

static TokenKind keyword_or_identifier(std::string_view word)
{
    const auto it = keywords.find(word);

    if (it != keywords.end())
        return it->second;

    return TK::Ident;
}

static const char *tok_kind_string(TokenKind kind)
{
    switch (kind) {
    case TK::Eof: return "EOF";
    case TK::Unknown: return "Unknown";
    case TK::IntNum: return "IntNum";
    case TK::Ident: return "Ident";

    case TK::Equal: return "=";
    case TK::Equal2: return "==";
    case TK::Plus: return "+";
    case TK::Minus: return "-";
    case TK::Slash: return "/";
    case TK::Period: return ".";
    case TK::Hash: return "#";
    case TK::Hash2: return "##";

    case TK::Int: return "int";
    case TK::If: return "if";
    case TK::Else: return "else";
    case TK::Return: return "return";
    case TK::String: return "string";

    case TK::Comma: return ",";
    case TK::LParen: return "(";
    case TK::RParen: return ")";
    case TK::BlockBegin: return "BlockBegin";
    case TK::BlockEnd: return "BlockEnd";
    case TK::NewLine: return "\\n";

    default:
        ERROR_NO_CASE(kind);
        return nullptr;
    }
}

std::ostream &operator<<(std::ostream &os, TokenKind kind)
{
    return os << tok_kind_string(kind);
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
        tok->kind = TK::BlockEnd;
        tok->pos = pos_;
        return;
    }

    if (is_line_begin_) {
        is_line_begin_ = false;

        const TokenKind kind = scan_indent(tok);
        if (kind == TK::BlockBegin || kind == TK::BlockEnd)
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

        if (ch == '=') {
            ch = get();

            if (ch == '=') {
                tok->kind = TK::Equal2;
                tok->pos = pos;
            }
            else {
                unget();
                tok->kind = TK::Equal;
                tok->pos = pos;
            }
            return;
        }

        if (ch == '+') {
            tok->kind = TK::Plus;
            tok->pos = pos;
            return;
        }

        if (ch == '-') {
            tok->kind = TK::Minus;
            tok->pos = pos;
            return;
        }

        if (ch == '/') {
            ch = get();

            if (ch == '/') {
                scan_line_comment();
                continue;
            }
            else {
                unget();
                tok->kind = TK::Slash;
                tok->pos = pos;
            }
            return;
        }

        if (ch == '.') {
            tok->kind = TK::Period;
            tok->pos = pos;
            return;
        }

        if (ch == ',') {
            tok->kind = TK::Comma;
            tok->pos = pos;
            return;
        }

        if (ch == '(') {
            tok->kind = TK::LParen;
            tok->pos = pos;
            return;
        }

        if (ch == ')') {
            tok->kind = TK::RParen;
            tok->pos = pos;
            return;
        }

        // word
        if (isalpha(ch)) {
            unget();
            scan_word(tok, pos);
            return;
        }

        if (ch == '#') {
            ch = get();

            if (ch == '#') {
                tok->kind = TK::Hash2;
                tok->pos = pos;
            }
            else {
                unget();
                tok->kind = TK::Hash;
                tok->pos = pos;
            }
            return;
        }

        if (ch == '\n') {
            tok->kind = TK::NewLine;
            tok->pos = pos;
            is_line_begin_ = true;
            return;
        }

        if (ch == EOF) {
            tok->kind = TK::Eof;
            tok->pos = pos;
            return;
        }

        // skip
        if (ch == ' ' || ch == '\t' || ch == '\v') {
            continue;
        }

        tok->kind = TK::Unknown;
        return;
    }

    tok->kind = TK::Eof;
    tok->pos = pos_;
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
    return isdigit(ch) || ishex(ch);
}

void Lexer::scan_number(Token *tok, Pos pos)
{
    auto start = it_;
    int base = 10;
    int len = 0;

    for (int ch = get(); isnum(ch); ch = get()) {
        if (ishex(ch))
            base = 16;
        len++;
    }

    unget();

    char *end = nullptr;

    tok->ival = strtol(&(*start), &end, base);
    tok->kind = TK::IntNum;
    tok->pos = pos;

    assert(end && (len == (end - &(*start))));
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

    tok->kind = keyword_or_identifier(word);
    if (tok->kind == TK::Ident)
        tok->sval = word;
    tok->pos = pos;
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
        tok->kind = TK::BlockBegin;
        tok->pos = pos_;

        //BlockBegin alwasy starts at beginning of line
        tok->pos.x = 1;

        return tok->kind;
    }
    else if (indent < indent_stack_.top()) {
        // pop indents until it matches current
        unread_blockend_ = 0;

        while (indent < indent_stack_.top()) {
            indent_stack_.pop();

            if (indent == indent_stack_.top()) {
                tok->kind = TK::BlockEnd;
                tok->pos = pos_;
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
