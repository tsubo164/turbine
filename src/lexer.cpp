#include "lexer.h"
#include <unordered_map>

static const std::unordered_map<std::string, TokenKind> keywords = {
    {"int", TK::Int},
    {"if", TK::If},
    {"else", TK::Else},
    {"return", TK::Return},
    {"string", TK::String},
};

static TokenKind keyword_or_identifier(const std::string &word)
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
        std::cerr << "TokenKind: " << static_cast<int>(kind)
            << " not in tok_kind_string()" << std::endl;
        std::exit(EXIT_FAILURE);
        return nullptr;
    }
}

std::ostream &operator<<(std::ostream &os, TokenKind kind)
{
    return os << tok_kind_string(kind);
}

Lexer::Lexer(StringTable &strtab) : strtable_(strtab)
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
            scan_number(ch, tok);
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
            scan_word(ch, tok);
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

void Lexer::scan_number(int first_char, Token *tok)
{
    strbuf_.clear();

    for (int ch = first_char; isdigit(ch); ch = get())
        strbuf_ += ch;

    unget();

    size_t pos = 0;
    tok->ival = std::stol(strbuf_, &pos, 10);
    tok->kind = TK::IntNum;
}

void Lexer::scan_word(int first_char, Token *tok)
{
    strbuf_.clear();

    for (int ch = first_char; isalnum(ch) || ch == '_'; ch = get())
        strbuf_ += ch;

    unget();

    tok->kind = keyword_or_identifier(strbuf_);
    if (tok->kind == TK::Ident)
        tok->sval = strtable_.Insert(strbuf_);
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
        std::exit(EXIT_FAILURE);
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
