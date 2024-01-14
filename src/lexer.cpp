#include "compiler.h"

#include "lexer.h"
#include "error.h"
#include "escseq.h"

#include <string>
#include <stack>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const KindInfo table[] = {
    { T_NUL,        "nul" },
    // type
    { T_keyword_begin, "keyword_begin" },
    { T_NIL,        "nil" },
    { T_TRU,        "true" },
    { T_FLS,        "false" },
    { T_BOL,        "bool" },
    { T_INT,        "int" },
    { T_FLT,        "float" },
    { T_STR,        "string" },
    // stmt
    { T_IF,         "if" },
    { T_FOR,        "for" },
    { T_ELS,        "or" },
    { T_BRK,        "break" },
    { T_CNT,        "continue" },
    { T_SWT,        "switch" },
    { T_CASE,       "case" },
    { T_DFLT,       "default" },
    { T_RET,        "return" },
    { T_NOP,        "nop" },
    { T_EXPR,       "expr" },
    { T_BLOCK,      "block" },
    // special
    { T_CALLER_LINE, "$caller_line" },
    { T_keyword_end, "keyword_end" },
    // list
    { T_EXPRLIST,   "xpr_list" },
    // identifier
    { T_FIELD,      "field" },
    { T_IDENT,      "ident" },
    { T_FUNC,       "func" },
    { T_VAR,        "var" },
    // literal
    { T_NILLIT,     "nil_lit" },
    { T_BOLLIT,     "bool_lit" },
    { T_INTLIT,     "int_lit" },
    { T_FLTLIT,     "float_lit" },
    { T_STRLIT,     "string_lit" },
    // separator
    { T_LPAREN,     "(" },
    { T_RPAREN,     ")" },
    { T_LBRACK,     "[" },
    { T_RBRACK,     "]" },
    { T_SEM,        ";" },
    { T_BLOCKBEGIN,     "block_begin" },
    { T_BLOCKEND,   "block_end" },
    { T_DASH3,      "---" },
    { T_DOT,        "." },
    { T_COMMA,      "," },
    { T_HASH,       "#" },
    { T_HASH2,      "##" },
    { T_NEWLINE,    "\\n" },
    // binary
    { T_ADD,        "+" },
    { T_SUB,        "-" },
    { T_MUL,        "*" },
    { T_DIV,        "/" },
    { T_REM,        "%" },
    // relational
    { T_EQ,         "==" },
    { T_NEQ,        "!=" },
    { T_LT,         "<" },
    { T_LTE,        "<=" },
    { T_GT,         ">" },
    { T_GTE,        ">=" },
    // bitwise
    { T_SHL,        "<<" },
    { T_SHR,        ">>" },
    { T_OR,         "|" },
    { T_XOR,        "^" },
    { T_AND,        "&" },
    { T_LOR,        "||" },
    { T_LAND,       "&&" },
    // array, struct, func
    { T_SELECT,     "select" },
    { T_INDEX,      "index" },
    { T_CALL,       "call" },
    // unary
    { T_LNOT,       "!" },
    { T_POS,        "+(pos)" },
    { T_NEG,        "-(neg)" },
    { T_ADR,        "&(addr)" },
    { T_DRF,        "*(deref)" },
    { T_NOT,        "~" },
    { T_INC,        "++" },
    { T_DEC,        "--" },
    { T_CONV,       "conversion" },
    // assign
    { T_ASSN,       "=" },
    { T_AADD,       "+=" },
    { T_ASUB,       "-=" },
    { T_AMUL,       "*=" },
    { T_ADIV,       "/=" },
    { T_AREM,       "%=" },
    // eof
    { T_EOF,        "EOF" },
};

static_assert(sizeof(table)/sizeof(table[0])==T_EOF+1, "MISSING_TOKEN_STRING");

const KindInfo *LookupKindInfo(int kind)
{
    int N = sizeof(table)/sizeof(table[0]);

    for (int i = 0; i < N; i++) {
        if (kind == table[i].kind)
            return &table[i];
    }
    return &table[0];
}

static int keyword_or_ident(const char *word)
{
    for (int i = T_keyword_begin + 1; i < T_keyword_end; i++) {
        const KindInfo *info = &table[i];
        if (!strcmp(word, info->str))
            return info->kind;
    }
    return T_IDENT;
}

const char *TokenKindString(int kind)
{
    const KindInfo *info = LookupKindInfo(kind);
    return info->str;
}

void set(Token *t, int k, Pos p)
{
    t->kind = k;
    t->pos = p;
}

typedef struct Lexer {
    // src text
    const char *src_;
    const char *it_;
    Pos pos_;
    int prevx = 0;

    // indent
    std::stack <int>indent_stack_;
    int unread_blockend_ = 0;
    bool is_line_begin_ = true;
} Lexer;

void SetInput(Lexer *l, const std::string &src)
{
    l->src_ = src.c_str();

    // init
    l->it_ = l->src_;
    l->pos_ = {0, 1};
    l->indent_stack_.push(0);
    l->is_line_begin_ = true;
}

int curr(const Lexer *l)
{
    if (l->it_ == l->src_)
        return '\0';
    else
        return *(l->it_ - 1);
}

int get(Lexer *l)
{
    l->prevx = l->pos_.x;

    if (curr(l) == '\n') {
        l->pos_.x = 1;
        l->pos_.y++;
    }
    else {
        l->pos_.x++;
    }

    return *l->it_++;
}

int peek(const Lexer *l)
{
    return *l->it_;
}

void unget(Lexer *l)
{
    l->it_--;

    if (curr(l) == '\n') {
        l->pos_.x = l->prevx;
        l->prevx--;
        l->pos_.y--;
    }
    else {
        l->pos_.x--;
    }
}

bool eof(const Lexer *l)
{
    return *l->it_ == '\0';
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

void scan_number(Lexer *l, Token *tok, Pos pos)
{
    const char *start = l->it_;
    bool fpnum = false;
    int base = 10;
    int len = 0;

    for (int ch = get(l); isnum(ch); ch = get(l)) {
        if (ishex(ch))
            base = 16;

        if (isfp(ch))
            fpnum = true;

        if (ch == 'e' || ch == 'E') {
            if (peek(l) == '-' || peek(l) == '+') {
                continue;
            }
            else {
                // reject 'e'/'E'
                unget(l);
                break;
            }
        }

        len++;
    }

    unget(l);

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

void scan_char_literal(Lexer *l, Token *tok, Pos pos)
{
    int ch = get(l);

    if (ch == '\\') {
        const int next = get(l);
        const bool found = FindEscapedChar(next, ch);
        if (!found) {
            unget(l);
            Error("unknown escape sequence", l->src_, l->pos_);
        }
    }

    tok->ival = ch;
    set(tok, T_INTLIT, pos);

    ch = get(l);
    if (ch != '\'') {
        unget(l);
        Error("unterminated char literal", l->src_, l->pos_);
    }
}

static bool isword(int ch)
{
    return isalnum(ch) || ch == '_';
}

void scan_word(Lexer *l, Token *tok, Pos pos)
{
    static char buf[128] = {'\0'};
    char *p = buf;
    int len = 0;

    const int first = get(l);
    if (first == '$' || isword(first)) {
        *p++ = first;
        len++;
    }

    for (int ch = get(l); isword(ch); ch = get(l)) {
        *p++ = ch;
        len++;
        if (len == sizeof(buf)) {
            // error
        }
    }
    *p = '\0';

    unget(l);

    const int kind = keyword_or_ident(buf);
    tok->sval = intern(buf);
    set(tok, kind, pos);
}

void scan_string(Lexer *l, Token *tok, Pos pos)
{
    static char buf[4096] = {'\0'};
    char *p = buf;

    const Pos strpos = pos;
    auto start = l->it_;
    int len = 0;
    int backslashes = 0;

    for (int ch = get(l); ch != '"'; ch = get(l)) {
        const int next = peek(l);

        if (ch == '\\') {
            backslashes++;
            if (next == '"' || next == '\\') {
                *p++ = ch;
                len++;

                ch = get(l);
            }
        }

        if (ch == EOF || ch == '\0') {
            unget(l);
            Error("unterminated string literal", l->src_, strpos);
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

void scan_line_comment(Lexer *l)
{
    for (;;) {
        const int ch = get(l);

        if (ch == '\n') {
            unget(l);
            break;
        }
    }
}

void scan_block_comment(Lexer *l, Pos pos)
{
    const Pos commentpos = pos;
    // already accepted "/*"
    int depth = 1;

    for (;;) {
        int ch = get(l);

        if (ch == '/') {
            ch = get(l);
            if (ch == '*') {
                depth++;
                continue;
            }
        }

        if (ch == '*') {
            ch = get(l);
            if (ch == '/') {
                depth--;
                if (depth == 0)
                    break;
                else
                    continue;
            }
        }

        if (ch == EOF || ch == '\0') {
            unget(l);
            Error("unterminated block comment", l->src_, commentpos);
        }
    }
}

int count_indent(Lexer *l)
{
    int indent = 0;

    for (;;) {
        const int ch = get(l);

        if (ch == '/') {
            // indent + line comment => next line
            if (peek(l) == '/') {
                scan_line_comment(l);
                continue;
            }
            else {
                unget(l);
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
            unget(l);
            break;
        }
    }

    return indent;
}

int scan_indent(Lexer *l, Token *tok)
{
    const int indent = count_indent(l);

    if (indent > l->indent_stack_.top()) {
        // push indent
        l->indent_stack_.push(indent);
        set(tok, T_BLOCKBEGIN, l->pos_);

        // BlockBegin alwasy starts at beginning of line
        tok->pos.x = 1;

        return tok->kind;
    }
    else if (indent < l->indent_stack_.top()) {
        // pop indents until it matches current
        l->unread_blockend_ = 0;

        while (indent < l->indent_stack_.top()) {
            l->indent_stack_.pop();

            if (indent == l->indent_stack_.top()) {
                set(tok, T_BLOCKEND, l->pos_);
                return tok->kind;
            }

            l->unread_blockend_++;
        }

        // no indent matches current
        Error("mismatch outer indent", l->src_, l->pos_);
        return tok->kind;
    }
    else {
        // no indent change
        return tok->kind;
    }
}

void Get(Lexer *l, Token *tok)
{
    *tok = {};

    if (l->unread_blockend_ > 0) {
        l->unread_blockend_--;
        set(tok, T_BLOCKEND, l->pos_);
        return;
    }

    if (l->is_line_begin_) {
        l->is_line_begin_ = false;

        const int kind = scan_indent(l, tok);
        if (kind == T_BLOCKBEGIN || kind == T_BLOCKEND)
            return;
    }

    while (!eof(l)) {
        int ch = get(l);
        const Pos pos = l->pos_;

        // number
        if (isdigit(ch)) {
            unget(l);
            scan_number(l, tok, pos);
            return;
        }

        if (ch == '\'') {
            scan_char_literal(l, tok, pos);
            return;
        }

        if (ch == '=') {
            ch = get(l);

            if (ch == '=') {
                set(tok, T_EQ, pos);
            }
            else {
                unget(l);
                set(tok, T_ASSN, pos);
            }
            return;
        }

        if (ch == '!') {
            ch = get(l);

            if (ch == '=') {
                set(tok, T_NEQ, pos);
            }
            else {
                unget(l);
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
            ch = get(l);
            if (ch == '<') {
                set(tok, T_SHL, pos);
            }
            else if (ch == '=') {
                set(tok, T_LTE, pos);
            }
            else {
                unget(l);
                set(tok, T_LT, pos);
            }
            return;
        }

        if (ch == '>') {
            ch = get(l);
            if (ch == '>') {
                set(tok, T_SHR, pos);
            }
            else if (ch == '=') {
                set(tok, T_GTE, pos);
            }
            else {
                unget(l);
                set(tok, T_GT, pos);
            }
            return;
        }

        if (ch == '+') {
            ch = get(l);
            if (ch == '+') {
                set(tok, T_INC, pos);
            }
            else if (ch == '=') {
                set(tok, T_AADD, pos);
            }
            else {
                unget(l);
                set(tok, T_ADD, pos);
            }
            return;
        }

        if (ch == '-') {
            ch = get(l);
            if (ch == '-') {
                //set(tok, T_MINUS2, pos);
                ch = get(l);
                if (ch == '-') {
                    set(tok, T_DASH3, pos);
                }
                else {
                    unget(l);
                    set(tok, T_DEC, pos);
                }
            }
            else if (ch == '=') {
                set(tok, T_ASUB, pos);
            }
            else {
                unget(l);
                set(tok, T_SUB, pos);
            }
            return;
        }

        if (ch == '*') {
            ch = get(l);
            if (ch == '=') {
                set(tok, T_AMUL, pos);
            }
            else {
                unget(l);
                set(tok, T_MUL, pos);
            }
            return;
        }

        if (ch == '/') {
            ch = get(l);
            if (ch == '/') {
                scan_line_comment(l);
                continue;
            }
            else if (ch == '*') {
                scan_block_comment(l, pos);
                continue;
            }
            else if (ch == '=') {
                set(tok, T_ADIV, pos);
            }
            else {
                unget(l);
                set(tok, T_DIV, pos);
            }
            return;
        }

        if (ch == '%') {
            ch = get(l);
            if (ch == '=') {
                set(tok, T_AREM, pos);
            }
            else {
                unget(l);
                set(tok, T_REM, pos);
            }
            return;
        }

        if (ch == '|') {
            ch = get(l);
            if (ch == '|') {
                set(tok, T_LOR, pos);
            }
            else {
                unget(l);
                set(tok, T_OR, pos);
            }
            return;
        }

        if (ch == '&') {
            ch = get(l);
            if (ch == '&') {
                set(tok, T_LAND, pos);
            }
            else {
                unget(l);
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
            unget(l);
            scan_word(l, tok, pos);
            return;
        }

        if (ch == '$') {
            unget(l);
            scan_word(l, tok, pos);
            if (tok->kind == T_IDENT) {
                const std::string msg =
                    "unknown special variables: '$" +
                    std::string(tok->sval) + "'";
                Error(msg, l->src_, pos);
            }
            return;
        }

        // string
        if (ch == '"') {
            scan_string(l, tok, pos);
            return;
        }

        if (ch == '#') {
            ch = get(l);
            if (ch == '#') {
                set(tok, T_HASH2, pos);
            }
            else {
                unget(l);
                set(tok, T_HASH, pos);
            }
            return;
        }

        if (ch == '\n') {
            set(tok, T_NEWLINE, pos);
            l->is_line_begin_ = true;
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

        Error("unknown token", l->src_, l->pos_);
        return;
    }

    set(tok, T_EOF, l->pos_);
}

const Token *Tokenize(const char *src)
{
    std::string tmp(src);
    Lexer l;
    SetInput(&l, tmp);

    Token *head = CALLOC(Token);
    Token *tail = head;

    for (;;) {
        Token *t = CALLOC(Token);
        Get(&l, t);

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
