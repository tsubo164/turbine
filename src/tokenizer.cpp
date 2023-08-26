#include "tokenizer.h"
#include <unordered_map>

static const std::unordered_map<std::string, TokenKind> keywords = {
    {"int", TK::Int},
    {"if", TK::If},
    {"return", TK::Return},
};

static TokenKind keyword_or_identifier(const std::string &word)
{
    const auto found = keywords.find(word);
    if (found != keywords.end()) {
        return found->second;
    }
    return TK::Ident;
}

static const char *tok_kind_string(TokenKind kind)
{
#define T(kind) case TK::kind: return #kind;
    switch (kind) {
    T(Eof);
    T(Unknown);
    T(IntNum);
    T(Ident);

    T(Equal);
    T(Equal2);
    T(Plus);
    T(Minus);
    T(Hash);

    T(Int);
    T(If);
    T(Return);

    T(Comma);
    T(LeftParenthesis);
    T(RightParenthesis);
    T(BlockBegin);
    T(BlockEnd);
    T(NewLine);
    default:
        std::cerr << "TokenKind: " << static_cast<int>(kind)
            << " not in tok_kind_string()" << std::endl;
        std::exit(EXIT_FAILURE);
        return nullptr;
    }
#undef T
}

std::ostream &operator<<(std::ostream &os, TokenKind kind)
{
    return os << tok_kind_string(kind);
}

Tokenizer::Tokenizer(StringTable &string_table) : strtable_(string_table)
{
    indent_stack_.push(0);
    is_line_begin_ = true;
}

Tokenizer::~Tokenizer()
{
}

void Tokenizer::SetInput(std::istream &stream)
{
    stream_ = &stream;
}

void Tokenizer::Get(Token *tok)
{
    *tok = {};

    if (is_line_begin_) {
        is_line_begin_ = false;

        const int indent = scan_indent();
        if (indent > indent_stack_.top()) {
            indent_stack_.push(indent);
            tok->kind = TK::BlockBegin;
            return;
        }
        else if (indent < indent_stack_.top()) {
            unread_blockend_ = 0;
            while (indent < indent_stack_.top()) {
                indent_stack_.pop();
                if (indent == indent_stack_.top()) {
                    tok->kind = TK::BlockEnd;
                    return;
                }
                unread_blockend_++;
            }
            std::cerr << "mismatch outer indent" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    while (!stream_->eof()) {
        int ch = stream_->get();

        // number
        if (isdigit(ch)) {
            scan_number(ch, tok);
            return;
        }

        if (ch == '=') {
            ch = stream_->get();

            if (ch == '=') {
                tok->kind = TK::Equal2;
            }
            else {
                stream_->unget();
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

        if (ch == ',') {
            tok->kind = TK::Comma;
            return;
        }

        if (ch == '(') {
            tok->kind = TK::LeftParenthesis;
            return;
        }

        if (ch == ')') {
            tok->kind = TK::RightParenthesis;
            return;
        }

        // word
        if (isalpha(ch)) {
            scan_word(ch, tok);
            return;
        }

        if (ch == '#') {
            tok->kind = TK::Hash;
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

TokenKind Tokenizer::scan_number(int first_char, Token *tok)
{
    static char buf[256] = {'\0'};
    char *pbuf = buf;

    for (int ch = first_char; isdigit(ch); ch = stream_->get()) {
        *pbuf++ = ch;
    }
    stream_->unget();

    *pbuf = '\0';
    pbuf = buf;

    char *end = nullptr;
    tok->ival = strtol(buf, &end, 10);
    tok->kind = TK::IntNum;

    return tok->kind;
}

TokenKind Tokenizer::scan_word(int first_char, Token *tok)
{
    strbuf_.clear();

    for (int ch = first_char; isalnum(ch) || ch == '_'; ch = stream_->get()) {
        strbuf_ += ch;
    }
    stream_->unget();

    tok->kind = keyword_or_identifier(strbuf_);
    if (tok->kind == TK::Ident)
        tok->sval = strtable_.Insert(strbuf_);

    return tok->kind;
}

int Tokenizer::scan_indent()
{
    int indent = 0;

    for (;;) {
        const int ch = stream_->get();

        if (ch == ' ' || ch == '\v' || ch == '\f') {
            indent++;
            continue;
        }
        else if (ch == '\t') {
            indent += 4;
            continue;
        }
        else if (ch == '\n') {
            // blank line, move to next line
            indent = 0;
            continue;
        }
        else {
            stream_->unget();
            break;
        }
    }

    return indent;
}
