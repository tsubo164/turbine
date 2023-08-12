#include "tokenizer.h"

Tokenizer::Tokenizer()
{
}

Tokenizer::~Tokenizer()
{
}

void Tokenizer::SetInput(std::istream &stream)
{
    strm_ = &stream;
}

int Tokenizer::Get(Token &tok)
{
    while (!strm_->eof()) {
        const int ch = strm_->get();

        switch (ch) {

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            scan_number(ch, tok);
            return tok.kind;

        case EOF:
            tok.kind = TOK_EOF;
            return tok.kind;

        default:
            break;
        }
    }
    return TOK_UNKNOWN;
}

int Tokenizer::scan_number(int first_char, Token &tok)
{
    static char buf[256] = {'\0'};
    char *pbuf = buf;

    for (int ch = first_char; isdigit(ch); ch = strm_->get()) {
        *pbuf++ = ch;
    }
    strm_->unget();

    *pbuf = '\0';
    pbuf = buf;

    char *end = NULL;
    tok.ival = strtol(buf, &end, 10);
    tok.kind = TOK_INTNUM;

    return tok.kind;
}

