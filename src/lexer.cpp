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
}

int Lexer::get()
{
    return *it_++;
}

int Lexer::peek()
{
    return *it_;
}

void Lexer::unget()
{
    --it_;
}

bool Lexer::eof() const
{
    return it_ == src_->end();
}

void Lexer::Get(Token *tok)
{
    *tok = {};

    if (unread_blockend_ > 0) {
        unread_blockend_--;
        tok->kind = TK::BlockEnd;
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

        // number
        if (isdigit(ch)) {
            unget();
            scan_number(tok);
            return;
        }

        if (ch == '=') {
            ch = get();

            if (ch == '=') {
                tok->kind = TK::Equal2;
            }
            else {
                unget();
                tok->kind = TK::Equal;
            }
            return;
        }

        if (ch == '+') {
            tok->kind = TK::Plus;
            return;
        }

        if (ch == '-') {
            tok->kind = TK::Minus;
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
            }
            return;
        }

        if (ch == '.') {
            tok->kind = TK::Period;
            return;
        }

        if (ch == ',') {
            tok->kind = TK::Comma;
            return;
        }

        if (ch == '(') {
            tok->kind = TK::LParen;
            return;
        }

        if (ch == ')') {
            tok->kind = TK::RParen;
            return;
        }

        // word
        if (isalpha(ch)) {
            unget();
            scan_word(tok);
            return;
        }

        if (ch == '#') {
            ch = get();

            if (ch == '#') {
                tok->kind = TK::Hash2;
            }
            else {
                unget();
                tok->kind = TK::Hash;
            }
            return;
        }

        if (ch == '\n') {
            tok->kind = TK::NewLine;
            is_line_begin_ = true;
            return;
        }

        if (ch == EOF) {
            tok->kind = TK::Eof;
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

void Lexer::scan_number(Token *tok)
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

    assert(end && (len == (end - &(*start))));
}

static bool isword(int ch)
{
    return isalnum(ch) || ch == '_';
}

void Lexer::scan_word(Token *tok)
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
        return tok->kind;
    }
    else if (indent < indent_stack_.top()) {
        // pop indents until it matches current
        unread_blockend_ = 0;

        while (indent < indent_stack_.top()) {
            indent_stack_.pop();

            if (indent == indent_stack_.top()) {
                tok->kind = TK::BlockEnd;
                return tok->kind;
            }

            unread_blockend_++;
        }

        // no indent matches current
        std::cerr << "mismatch outer indent" << std::endl;
        exit(EXIT_FAILURE);
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
