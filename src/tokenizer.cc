#include "tokenizer.h"
#include <unordered_map>

static const std::unordered_map<std::string, int> keywords = {
    {"if", TOK_IF},
};

static int keyword_or_identifier(const std::string &word)
{
    const auto found = keywords.find(word);
    if (found != keywords.end()) {
        return found->second;
    }
    return TOK_IDENT;
}

Tokenizer::Tokenizer()
{
}

Tokenizer::~Tokenizer()
{
}

void Tokenizer::SetInput(std::istream &stream)
{
    stream_ = &stream;
}

void Tokenizer::SetStringTable(StringTable &string_table)
{
    strtable_ = &string_table;
}

int Tokenizer::Get(Token &tok)
{
    tok = {};

    while (!stream_->eof()) {
        const int ch = stream_->get();

        // number
        if (isdigit(ch)) {
            scan_number(ch, tok);
            break;
        }

        if (ch == '+') {
            tok.kind = TOK_PLUS;
            break;
        }

        // word
        if (isalpha(ch)) {
            scan_word(ch, tok);
            break;
        }

        if (ch == EOF) {
            tok.kind = TOK_EOF;
            break;
        }
    }

    return tok.kind;
}

int Tokenizer::scan_number(int first_char, Token &tok)
{
    static char buf[256] = {'\0'};
    char *pbuf = buf;

    for (int ch = first_char; isdigit(ch); ch = stream_->get()) {
        *pbuf++ = ch;
    }
    stream_->unget();

    *pbuf = '\0';
    pbuf = buf;

    char *end = NULL;
    tok.ival = strtol(buf, &end, 10);
    tok.kind = TOK_INTNUM;

    return tok.kind;
}

int Tokenizer::scan_word(int first_char, Token &tok)
{
    strbuf_.clear();

    for (int ch = first_char; isalnum(ch) || ch == '_'; ch = stream_->get()) {
        strbuf_ += ch;
    }
    stream_->unget();

    tok.kind = keyword_or_identifier(strbuf_);
    if (tok.kind == TOK_IDENT)
        tok.str_id = strtable_->Insert(strbuf_);

    return tok.kind;
}
