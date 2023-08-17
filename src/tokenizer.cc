#include "tokenizer.h"
#include <unordered_map>

static const std::unordered_map<std::string, TokenKind> keywords = {
    {"if", TK::If},
};

static TokenKind keyword_or_identifier(const std::string &word)
{
    const auto found = keywords.find(word);
    if (found != keywords.end()) {
        return found->second;
    }
    return TK::Ident;
}

void Tokenizer::SetInput(std::istream &stream)
{
    stream_ = &stream;
}

TokenKind Tokenizer::Get(Token &tok)
{
    tok = {};

    while (!stream_->eof()) {
        const int ch = stream_->get();

        // number
        if (isdigit(ch)) {
            scan_number(ch, tok);
            return tok.kind;
        }

        if (ch == '=') {
            tok.kind = TK::Equal;
            return tok.kind;
        }

        if (ch == '+') {
            tok.kind = TK::Plus;
            return tok.kind;
        }

        // word
        if (isalpha(ch)) {
            scan_word(ch, tok);
            return tok.kind;
        }

        if (ch == '\n') {
            tok.kind = TK::NewLine;
            return tok.kind;
        }

        if (ch == EOF) {
            tok.kind = TK::Eof;
            return tok.kind;
        }

        // skip
        if (ch == ' ' || ch == '\t' || ch == '\v') {
            continue;
        }

        tok.kind = TK::Unknown;
        return tok.kind;
    }

    tok.kind = TK::Eof;
    return tok.kind;
}

TokenKind Tokenizer::scan_number(int first_char, Token &tok)
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
    tok.ival = strtol(buf, &end, 10);
    tok.kind = TK::IntNum;

    return tok.kind;
}

TokenKind Tokenizer::scan_word(int first_char, Token &tok)
{
    strbuf_.clear();

    for (int ch = first_char; isalnum(ch) || ch == '_'; ch = stream_->get()) {
        strbuf_ += ch;
    }
    stream_->unget();

    tok.kind = keyword_or_identifier(strbuf_);
    if (tok.kind == TK::Ident)
        tok.str_id = strtable_.Insert(strbuf_);

    return tok.kind;
}
