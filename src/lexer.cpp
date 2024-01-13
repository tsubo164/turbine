#include "lexer.h"
#include "error.h"
#include "escseq.h"
#include "compiler.h"

#include <string>
#include <stack>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct TokInfo2 {
    int kind;
    const char *str;
    char type;
} TokInfo2;

static const TokInfo2 keywords[] = {
    { T_NIL,      "nil"      },
    { T_TRU,     "true"     },
    { T_FLS,    "false"    },
    { T_BOL,     "bool"     },
    { T_INT,      "int"      },
    { T_FLT,    "float"    },
    { T_STR,   "string"   },
    { T_IF,       "if"       },
    { T_OR,       "or"       },
    { T_ELS,     "else"     },
    { T_FOR,      "for"      },
    { T_BRK,    "break"    },
    { T_CNT, "continue" },
    { T_SWT,   "switch"   },
    { T_CASE,     "case"     },
    { T_DFLT,  "default"  },
    { T_RET,   "return"   },
    { T_NOP,      "nop"      },
    // special vars
    { T_CALLER_LINE, "$caller_line" },
    { T_NUL,  NULL,  },
};

static int keyword_or_identifier(const char *word)
{
    for (const TokInfo2 *info = keywords; info->kind; info++) {
        if (!strcmp(word, info->str))
            return info->kind;
    }
    return T_IDENT;
}

const char *TokenKindString(int kind)
{
    const TokInfo *info = find_tokinfo(kind);
    return info->str;
}

void set(Token *t, int k, Pos p)
{
    t->kind = k;
    t->pos = p;
}

class Lexer {
public:
    Lexer();
    ~Lexer();

    void SetInput(const std::string &src);
    void Get(Token *tok);

private:
    // src text
    const std::string *src_ {};
    std::string::const_iterator it_;
    Pos pos_;
    int prevx = 0;

    // indent
    std::stack <int>indent_stack_;
    int unread_blockend_ = 0;
    bool is_line_begin_ = true;

    int get();
    int peek();
    void unget();
    bool eof() const;
    int curr() const;

    void scan_number(Token *tok, Pos pos);
    void scan_char_literal(Token *tok, Pos pos);
    void scan_word(Token *tok, Pos pos);
    void scan_string(Token *tok, Pos pos);
    int count_indent();
    int scan_indent(Token *tok);
    void scan_line_comment();
    void scan_block_comment(Pos pos);
};

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
    pos_ = {0, 1};
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
        set(tok, T_BLOCKEND, pos_);
        return;
    }

    if (is_line_begin_) {
        is_line_begin_ = false;

        const int kind = scan_indent(tok);
        if (kind == T_BLOCKBEGIN || kind == T_BLOCKEND)
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
                set(tok, T_EQ, pos);
            }
            else {
                unget();
                set(tok, T_ASSN, pos);
            }
            return;
        }

        if (ch == '!') {
            ch = get();

            if (ch == '=') {
                set(tok, T_NEQ, pos);
            }
            else {
                unget();
                set(tok, T_LNOT, pos);
            }
            return;
        }

        if (ch == '^') {
            set(tok, T_XOR, pos);
            return;
        }

        if (ch == '~') {
            set(tok, T_NOT, pos);
            return;
        }

        if (ch == '<') {
            ch = get();
            if (ch == '<') {
                set(tok, T_SHL, pos);
            }
            else if (ch == '=') {
                set(tok, T_LTE, pos);
            }
            else {
                unget();
                set(tok, T_LT, pos);
            }
            return;
        }

        if (ch == '>') {
            ch = get();
            if (ch == '>') {
                set(tok, T_SHR, pos);
            }
            else if (ch == '=') {
                set(tok, T_GTE, pos);
            }
            else {
                unget();
                set(tok, T_GT, pos);
            }
            return;
        }

        if (ch == '+') {
            ch = get();
            if (ch == '+') {
                set(tok, T_INC, pos);
            }
            else if (ch == '=') {
                set(tok, T_AADD, pos);
            }
            else {
                unget();
                set(tok, T_ADD, pos);
            }
            return;
        }

        if (ch == '-') {
            ch = get();
            if (ch == '-') {
                //set(tok, T_MINUS2, pos);
                ch = get();
                if (ch == '-') {
                    set(tok, T_DASH3, pos);
                }
                else {
                    unget();
                    set(tok, T_DEC, pos);
                }
            }
            else if (ch == '=') {
                set(tok, T_ASUB, pos);
            }
            else {
                unget();
                set(tok, T_SUB, pos);
            }
            return;
        }

        if (ch == '*') {
            ch = get();
            if (ch == '=') {
                set(tok, T_AMUL, pos);
            }
            else {
                unget();
                set(tok, T_MUL, pos);
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
                set(tok, T_ADIV, pos);
            }
            else {
                unget();
                set(tok, T_DIV, pos);
            }
            return;
        }

        if (ch == '%') {
            ch = get();
            if (ch == '=') {
                set(tok, T_AREM, pos);
            }
            else {
                unget();
                set(tok, T_REM, pos);
            }
            return;
        }

        if (ch == '|') {
            ch = get();
            if (ch == '|') {
                set(tok, T_LOR, pos);
            }
            else {
                unget();
                set(tok, T_OR, pos);
            }
            return;
        }

        if (ch == '&') {
            ch = get();
            if (ch == '&') {
                set(tok, T_LAND, pos);
            }
            else {
                unget();
                set(tok, T_AND, pos);
            }
            return;
        }

        if (ch == '.') {
            set(tok, T_DOT, pos);
            return;
        }

        if (ch == ',') {
            set(tok, T_COMMA, pos);
            return;
        }

        if (ch == ';') {
            set(tok, T_SEM, pos);
            return;
        }

        if (ch == '(') {
            set(tok, T_LPAREN, pos);
            return;
        }

        if (ch == ')') {
            set(tok, T_RPAREN, pos);
            return;
        }

        if (ch == '[') {
            set(tok, T_LBRACK, pos);
            return;
        }

        if (ch == ']') {
            set(tok, T_RBRACK, pos);
            return;
        }

        // word
        if (isalpha(ch)) {
            unget();
            scan_word(tok, pos);
            return;
        }

        if (ch == '$') {
            unget();
            scan_word(tok, pos);
            if (tok->kind == T_IDENT) {
                const std::string msg =
                    "unknown special variables: '$" +
                    std::string(tok->sval) + "'";
                Error(msg, *src_, pos);
            }
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
                set(tok, T_HASH2, pos);
            }
            else {
                unget();
                set(tok, T_HASH, pos);
            }
            return;
        }

        if (ch == '\n') {
            set(tok, T_NEWLINE, pos);
            is_line_begin_ = true;
            return;
        }

        if (ch == EOF) {
            set(tok, T_EOF, pos);
            return;
        }

        // skip
        if (ch == ' ' || ch == '\t' || ch == '\v') {
            continue;
        }

        Error("unknown token", *src_, pos_);
        return;
    }

    set(tok, T_EOF, pos_);
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
        set(tok, T_FLTLIT, pos);
    }
    else {
        tok->ival = strtol(&(*start), &end, base);
        set(tok, T_INTLIT, pos);
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
    set(tok, T_INTLIT, pos);

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
    static char buf[128] = {'\0'};
    char *p = buf;
    int len = 0;

    const int first = get();
    if (first == '$' || isword(first)) {
        *p++ = first;
        len++;
    }

    for (int ch = get(); isword(ch); ch = get()) {
        *p++ = ch;
        len++;
        if (len == sizeof(buf)) {
            // error
        }
    }
    *p = '\0';

    unget();

    const int kind = keyword_or_identifier(buf);
    tok->sval = intern(buf);
    set(tok, kind, pos);
}

void Lexer::scan_string(Token *tok, Pos pos)
{
    static char buf[4096] = {'\0'};
    char *p = buf;

    const Pos strpos = pos;
    auto start = it_;
    int len = 0;
    int backslashes = 0;

    for (int ch = get(); ch != '"'; ch = get()) {
        const int next = peek();

        if (ch == '\\') {
            backslashes++;
            if (next == '"' || next == '\\') {
                *p++ = ch;
                len++;

                ch = get();
            }
        }

        if (ch == EOF || ch == '\0') {
            unget();
            Error("unterminated string literal", *src_, strpos);
        }

        *p++ = ch;
        len++;
    }
    *p = '\0';

    const std::string_view str_lit(&(*start), len);

    tok->has_escseq = backslashes > 0;
    tok->sval = intern(buf);
    set(tok, T_STRLIT, pos);
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

int Lexer::scan_indent(Token *tok)
{
    const int indent = count_indent();

    if (indent > indent_stack_.top()) {
        // push indent
        indent_stack_.push(indent);
        set(tok, T_BLOCKBEGIN, pos_);

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
                set(tok, T_BLOCKEND, pos_);
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

const Token *Tokenize(const char *src)
{
    std::string tmp(src);
    Lexer lexer;
    lexer.SetInput(tmp);

    Token *head = CALLOC(Token);
    Token *tail = head;

    for (;;) {
        Token *t = CALLOC(Token);
        lexer.Get(t);

        tail->next = t;
        t->prev = tail;
        tail = t;

        if (t->kind == T_EOF)
            break;
    }

    return head;
}

void PrintToken(const Token *token, bool format)
{
    int indent = 0;
    bool bol = true;

    for (const Token *tok = token; tok; tok = tok->next) {

        if (!format) {
            printf("(%4d, %3d) %s",
                    tok->pos.y, tok->pos.x,
                    TokenKindString(tok->kind));

            if (tok->kind == T_IDENT)
                printf(" (%s)", tok->sval);
            if (tok->kind == T_INTLIT)
                printf(" (%ld)", tok->ival);
            if (tok->kind == T_FLTLIT)
                printf(" (%g)", tok->fval);
            if (tok->kind == T_STRLIT)
                printf(" (\"%s\")", tok->sval);

            printf("\n");
        }
        else {
            if (tok->kind == T_BLOCKBEGIN) {
                indent++;
                continue;
            }
            else if (tok->kind == T_BLOCKEND) {
                indent--;
                continue;
            }

            if (bol) {
                bol = false;
                for (int i = 0; i < indent; i++)
                    printf("....");
            }

            if (tok->kind == T_NEWLINE) {
                printf("%s\n", TokenKindString(tok->kind));
                bol = true;
            }
            else if (tok->kind != T_BLOCKBEGIN && tok->kind != T_BLOCKEND) {
                printf("%s ", TokenKindString(tok->kind));
            }
        }

        if (tok->kind == T_EOF)
            break;
    }
}
