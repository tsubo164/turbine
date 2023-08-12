#include <iostream>
#include <sstream>
#include <cctype>

enum TokenKind {
    TOK_UNKNOWN = -1,
    TOK_INTNUM,
    TOK_EOF
};

struct Token {
    int kind = 0;
    long ival;
};

class Tokenizer {
public:
    Tokenizer() {}
    ~Tokenizer() {}

    void SetInput(std::istream &sstrm);
    int Get(Token &tok);

private:
    std::istream *strm = nullptr;

    int scan_number(int first_char, Token &tok);
};

void Tokenizer::SetInput(std::istream &stream)
{
    strm = &stream;
}

int Tokenizer::Get(Token &tok)
{
    while (!strm->eof()) {
        const int ch = strm->get();

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

    for (int ch = first_char; isdigit(ch); ch = strm->get()) {
        *pbuf++ = ch;
    }
    strm->unget();

    *pbuf = '\0';
    pbuf = buf;

    char *end = NULL;
    tok.ival = strtol(buf, &end, 10);
    tok.kind = TOK_INTNUM;

    return tok.kind;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        return -1;

    std::stringstream strm(argv[1]);
    Tokenizer toknizer;
    Token tok;

    toknizer.SetInput(strm);
    toknizer.Get(tok);

    printf("%ld\n", tok.ival);

    return 0;
}
