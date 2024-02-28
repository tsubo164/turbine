#include "token.h"
#include "intern.h"
#include "escseq.h"
#include "error.h"
#include "mem.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static const struct KindInfo table[] = {
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
    // identifier
    { T_FIELD,      "field", 'y' },
    { T_IDENT,      "ident", 'y' },
    { T_FUNC,       "func",  'y' },
    { T_VAR,        "var",   'y' },
    // literal
    { T_NILLIT,     "nil_lit" },
    { T_BOLLIT,     "bool_lit",   'i' },
    { T_INTLIT,     "int_lit",    'i' },
    { T_FLTLIT,     "float_lit",  'f' },
    { T_STRLIT,     "string_lit", 's' },
    { T_FUNCLIT,    "func_lit",   'F' },
    // separator
    { T_LPAREN,     "(" },
    { T_RPAREN,     ")" },
    { T_LBRACK,     "[" },
    { T_RBRACK,     "]" },
    { T_SEM,        ";" },
    { T_COLON,      ":" },
    { T_COLON2,     "::" },
    { T_BLOCKBEGIN, "block_begin" },
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

const struct KindInfo *LookupKindInfo(int kind)
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
        const struct KindInfo *info = &table[i];
        if (!strcmp(word, info->str))
            return info->kind;
    }
    return T_IDENT;
}

const char *TokenString(int kind)
{
    const struct KindInfo *info = LookupKindInfo(kind);
    return info->str;
}

static void set(struct Token *t, int k, struct Pos p)
{
    t->kind = k;
    t->pos = p;
}

typedef struct Lexer {
    // src text
    const char *src;
    const char *it;
    struct Pos pos;
    int prevx;

    // indent
    int indent_stack[128];
    int sp;
    int unread_blockend;
    bool is_line_begin;
} Lexer;

static void error(const Lexer *l, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    VError(l->src, "fixme.ro", l->pos, fmt, args);
    va_end(args);
}

static void set_input(Lexer *l, const char *src)
{
    l->src = src;

    // init
    l->it = l->src;
    l->pos.x = 0;
    l->pos.y = 1;
    l->unread_blockend = 0;
    l->is_line_begin = true;

    l->indent_stack[0] = 0;
    l->sp = 0;
    l->prevx = 0;
}

static int curr(const Lexer *l)
{
    if (l->it == l->src)
        return '\0';
    else
        return *(l->it - 1);
}

static int get(Lexer *l)
{
    l->prevx = l->pos.x;

    if (curr(l) == '\n') {
        l->pos.x = 1;
        l->pos.y++;
    }
    else {
        l->pos.x++;
    }

    return *l->it++;
}

static int peek(const Lexer *l)
{
    return *l->it;
}

static void unget(Lexer *l)
{
    l->it--;

    if (curr(l) == '\n') {
        l->pos.x = l->prevx;
        l->prevx--;
        l->pos.y--;
    }
    else {
        l->pos.x--;
    }
}

static bool eof(const Lexer *l)
{
    return *l->it == '\0';
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

static void push(Lexer *l, int indent)
{
    if (l->sp == 127)
        error(l, "indent stack overflow");

    l->indent_stack[++l->sp] = indent;
}

static void pop(Lexer *l)
{
    l->sp--;
}

static int top(const Lexer *l)
{
    return l->indent_stack[l->sp];
}

static void scan_number(Lexer *l, struct Token *tok, struct Pos pos)
{
    const char *start = l->it;
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

    char *end = NULL;

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

static void scan_char_literal(Lexer *l, struct Token *tok, struct Pos pos)
{
    int ch = get(l);

    if (ch == '\\') {
        const int next = get(l);
        const bool found = FindEscapedChar(next, &ch);
        if (!found) {
            unget(l);
            error(l, "unknown escape sequence");
        }
    }

    tok->ival = ch;
    set(tok, T_INTLIT, pos);

    ch = get(l);
    if (ch != '\'') {
        unget(l);
        error(l, "unterminated char literal");
    }
}

static bool isword(int ch)
{
    return isalnum(ch) || ch == '_';
}

static void scan_word(Lexer *l, struct Token *tok, struct Pos pos)
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
    tok->sval = StrIntern(buf);
    set(tok, kind, pos);
}

static void scan_string(Lexer *l, struct Token *tok, struct Pos pos)
{
    static char buf[4096] = {'\0'};
    char *p = buf;

    const struct Pos strpos = pos;
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
            l->pos = strpos;
            error(l, "unterminated string literal");
        }

        *p++ = ch;
        len++;
    }
    *p = '\0';

    tok->has_escseq = backslashes > 0;
    tok->sval = StrIntern(buf);
    set(tok, T_STRLIT, pos);
}

static void scan_line_comment(Lexer *l)
{
    for (;;) {
        const int ch = get(l);

        if (ch == '\n') {
            unget(l);
            break;
        }
    }
}

static void scan_block_comment(Lexer *l, struct Pos pos)
{
    const struct Pos commentpos = pos;
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
            l->pos = commentpos;
            error(l, "unterminated block comment");
        }
    }
}

static int count_indent(Lexer *l)
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

static int scan_indent(Lexer *l, struct Token *tok)
{
    const int indent = count_indent(l);

    if (indent > top(l)) {
        // push indent
        push(l, indent);
        set(tok, T_BLOCKBEGIN, l->pos);

        // BlockBegin alwasy starts at beginning of line
        tok->pos.x = 1;

        return tok->kind;
    }
    else if (indent < top(l)) {
        // pop indents until it matches current
        l->unread_blockend = 0;

        while (indent < top(l)) {
            pop(l);

            if (indent == top(l)) {
                set(tok, T_BLOCKEND, l->pos);
                return tok->kind;
            }

            l->unread_blockend++;
        }

        // no indent matches current
        error(l, "mismatch outer indent");
        return tok->kind;
    }
    else {
        // no indent change
        return tok->kind;
    }
}

static void get_token(Lexer *l, struct Token *tok)
{
    const static struct Token ini = {0};
    *tok = ini;

    if (l->unread_blockend > 0) {
        l->unread_blockend--;
        set(tok, T_BLOCKEND, l->pos);
        return;
    }

    if (l->is_line_begin) {
        l->is_line_begin = false;

        const int kind = scan_indent(l, tok);
        if (kind == T_BLOCKBEGIN || kind == T_BLOCKEND)
            return;
    }

    while (!eof(l)) {
        int ch = get(l);
        const struct Pos pos = l->pos;

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

        if (ch == ':') {
            ch = get(l);
            if (ch == ':') {
                set(tok, T_COLON2, pos);
            }
            else {
                unget(l);
                set(tok, T_COLON, pos);
            }
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
                l->pos = pos;
                error(l, "unknown special variables: '%s'", tok->sval);
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
            l->is_line_begin = true;
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

        error(l, "unknown token");
        return;
    }

    set(tok, T_EOF, l->pos);
}

const struct Token *Tokenize(const char *src)
{
    Lexer l;
    set_input(&l, src);

    struct Token *head = CALLOC(struct Token);
    struct Token *tail = head;

    for (;;) {
        struct Token *t = CALLOC(struct Token);
        get_token(&l, t);

        tail->next = t;
        t->prev = tail;
        tail = t;

        if (t->kind == T_EOF)
            break;
    }

    return head;
}
