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
    switch (kind) {
    case TK::Eof: return "EOF";
    case TK::Unknown: return "Unknown";
    case TK::IntNum: return "IntNum";
    case TK::Ident: return "Ident";

    case TK::Equal: return "=";
    case TK::Equal2: return "==";
    case TK::Plus: return "+";
    case TK::Minus: return "-";
    case TK::Hash: return "#";

    case TK::Int: return "int";
    case TK::If: return "if";
    case TK::Return: return "return";

    case TK::Comma: return ",";
    case TK::LeftParenthesis: return "(";
    case TK::RightParenthesis: return ")";
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

        const TokenKind kind = scan_indent(tok);
        if (kind == TK::BlockBegin || kind == TK::BlockEnd)
            return;
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

int Tokenizer::count_indent()
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

TokenKind Tokenizer::scan_indent(Token *tok)
{
    const int indent = count_indent();

    if (indent > indent_stack_.top()) {
        indent_stack_.push(indent);
        tok->kind = TK::BlockBegin;
        return tok->kind;
    }
    else if (indent < indent_stack_.top()) {
        unread_blockend_ = 0;
        while (indent < indent_stack_.top()) {
            indent_stack_.pop();
            if (indent == indent_stack_.top()) {
                tok->kind = TK::BlockEnd;
                return tok->kind;
            }
            unread_blockend_++;
        }
        std::cerr << "mismatch outer indent" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    else {
        return tok->kind;
    }
}
