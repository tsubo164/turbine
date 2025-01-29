#include "parser_token.h"
#include "parser_escseq.h"
#include "parser_error.h"
#include "parser_limit.h"
#include "data_intern.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

const char *parser_get_token_string(int kind)
{
    static const char *table[] = {
    [TOK_ROOT]          = "root",
    [TOK_KEYWORD_BEGIN] = "keyword_begin",
    /* keyword */
    [TOK_NIL]           = "nil",
    [TOK_TRUE]          = "true",
    [TOK_FALSE]         = "false",
    [TOK_BOOL]          = "bool",
    [TOK_INT]           = "int",
    [TOK_FLOAT]         = "float",
    [TOK_STRING]        = "string",
    [TOK_STRUCT]        = "struct",
    [TOK_ENUM]          = "enum",
    [TOK_IF]            = "if",
    [TOK_ELSE]          = "or",
    [TOK_WHILE]         = "while",
    [TOK_FOR]           = "for",
    [TOK_IN]            = "in",
    [TOK_BREAK]         = "break",
    [TOK_CONTINUE]      = "continue",
    [TOK_SWITCH]        = "switch",
    [TOK_CASE]          = "case",
    [TOK_DEFAULT]       = "default",
    [TOK_RETURN]        = "return",
    [TOK_NOP]           = "nop",
    /* special */
    [TOK_CALLER_LINE]   = "$caller_line",
    [TOK_KEYWORD_END]   = "keyword_end",
    /* identifier */
    [TOK_IDENT]         = "ident",
    /* literal */
    [TOK_INTLIT]        = "int_lit",
    [TOK_FLOATLIT]      = "float_lit",
    [TOK_STRINGLIT]     = "string_lit",
    /* separator */
    [TOK_LPAREN]        = "(",
    [TOK_RPAREN]        = ")",
    [TOK_LBRACK]        = "[",
    [TOK_RBRACK]        = "]",
    [TOK_LBRACE]        = "{",
    [TOK_RBRACE]        = "}",
    [TOK_SEMICOLON]     = ";",
    [TOK_COLON]         = ":",
    [TOK_COLON2]        = "::",
    [TOK_BLOCKBEGIN]    = "block_begin",
    [TOK_BLOCKEND]      = "block_end",
    [TOK_MINUS3]        = "---",
    [TOK_PERIOD]        = ".",
    [TOK_PERIOD2]       = "..",
    [TOK_COMMA]         = ",",
    [TOK_HASH]          = "#",
    [TOK_HASH2]         = "##",
    [TOK_NEWLINE]       = "\\n",
    /* binary */
    [TOK_PLUS]          = "+",
    [TOK_MINUS]         = "-",
    [TOK_ASTER]         = "*",
    [TOK_SLASH]         = "/",
    [TOK_PERCENT]       = "%",
    /* relational */
    [TOK_EQUAL2]        = "==",
    [TOK_EXCLAMEQ]      = "!=",
    [TOK_LT]            = "<",
    [TOK_LTE]           = "<=",
    [TOK_GT]            = ">",
    [TOK_GTE]           = ">=",
    /* bitwise */
    [TOK_LT2]           = "<<",
    [TOK_GT2]           = ">>",
    [TOK_CARET]         = "^",
    [TOK_VBAR]          = "|",
    [TOK_VBAR2]         = "||",
    [TOK_AMPERSAND]     = "&",
    [TOK_AMPERSAND2]    = "&&",
    /* unary */
    [TOK_EXCLAM]        = "!",
    [TOK_TILDE]         = "~",
    /* assign */
    [TOK_EQUAL]         = "=",
    [TOK_PLUSEQ]        = "+=",
    [TOK_MINUSEQ]       = "-=",
    [TOK_ASTEREQ]       = "*=",
    [TOK_SLASHEQ]       = "/=",
    [TOK_PERCENTEQ]     = "%=",
    [TOK_LT2EQ]         = "<<=",
    [TOK_GT2EQ]         = ">>=",
    [TOK_CARETEQ]       = "^=",
    [TOK_VBAREQ]        = "|=",
    [TOK_AMPERSANDEQ]   = "&=",
    /* eof */
    [TOK_EOF]           = "EOF",
    };

    int count = sizeof(table) / sizeof(table[0]);
    assert(kind >= 0 && kind < count);

    return table[kind];
}

static int keyword_or_ident(const char *word)
{
    for (int i = TOK_KEYWORD_BEGIN + 1; i < TOK_KEYWORD_END; i++) {
        const char *keyword = parser_get_token_string(i);

        if (!strcmp(word, keyword))
            return i;
    }

    return TOK_IDENT;
}

static void set(struct parser_token *t, int k, struct parser_pos p)
{
    t->kind = k;
    t->pos = p;
}

struct lexer {
    /* src text */
    const char *src;
    const char *itr;
    struct parser_pos pos;
    int prevx;

    const char *filename;

    /* indent */
    int indent_stack[PARSER_MAX_INDENT_DEPTH];
    int sp;
    int unread_blockend;
    int grouping_depth;
    bool is_line_begin;
};

static void error(const struct lexer *l, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    parser_error_va(l->src, l->filename, l->pos.x, l->pos.y, fmt, args);
    va_end(args);
}

static void set_input(struct lexer *l, const char *src, const char *filename)
{
    l->src = src;
    l->filename = filename;

    /* init */
    l->itr = l->src;
    l->pos.x = 0;
    l->pos.y = 1;
    l->prevx = 0;

    l->indent_stack[0] = 0;
    l->sp = 0;
    l->unread_blockend = 0;
    l->grouping_depth = 0;

    l->is_line_begin = true;
}

static int curr(const struct lexer *l)
{
    if (l->itr == l->src)
        return '\0';
    else
        return *(l->itr - 1);
}

static int get(struct lexer *l)
{
    l->prevx = l->pos.x;

    if (curr(l) == '\n') {
        l->pos.x = 1;
        l->pos.y++;
    }
    else {
        l->pos.x++;
    }

    return *l->itr++;
}

static int peek(const struct lexer *l)
{
    return *l->itr;
}

static void unget(struct lexer *l)
{
    l->itr--;

    if (curr(l) == '\n') {
        l->pos.x = l->prevx;
        l->prevx--;
        l->pos.y--;
    }
    else {
        l->pos.x--;
    }
}

static bool eof(const struct lexer *l)
{
    return *l->itr == '\0';
}

static void push(struct lexer *l, int indent)
{
    if (l->sp == PARSER_MAX_INDENT_DEPTH - 1)
        error(l, "indent stack overflow");

    l->indent_stack[++l->sp] = indent;
}

static void pop(struct lexer *l)
{
    l->sp--;
}

static int top(const struct lexer *l)
{
    return l->indent_stack[l->sp];
}

/*
 * floating point number literals
 *   5.3876e4
 *   4e-11
 *   1e+5
 *   7.321E-3
 *   3.2E+4
 *   0.5e-6
 *   0.45
 *   6.e10
 */
static void scan_decimal(struct lexer *l, struct parser_token *tok, struct parser_pos pos)
{
    const char *start = l->itr;
    double fval = 0.0;
    long ival = 0;
    int fcount = 0;
    int icount = 0;
    int count = 0;
    int fnext = '\0';

    sscanf(start, "%le%n", &fval, &fcount);
    sscanf(start, "%li%n", &ival, &icount);

    /* to ensure scanning "for i in 0..N" */
    /*                              ^^^   */
    fnext = *(start + fcount);

    if (fcount > icount && fnext != '.') {
        tok->fval = fval;
        set(tok, TOK_FLOATLIT, pos);
        count = fcount;
    }
    else {
        tok->ival = ival;
        set(tok, TOK_INTLIT, pos);
        count = icount;
    }

    for (int i = 0; i < count; i++)
        get(l);
}

static void scan_char_literal(struct lexer *l, struct parser_token *tok, struct parser_pos pos)
{
    int ch = get(l);

    if (ch == '\\') {
        int next = get(l);
        int escseq = parser_find_escape_sequence(next);
        if (escseq != -1) {
            ch = escseq;
        }
        else {
            unget(l);
            error(l, "unknown escape sequence");
        }
    }

    tok->ival = ch;
    set(tok, TOK_INTLIT, pos);

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

static void validate_underscores(struct lexer *l, const char *name)
{
    int leading = 0;
    int trailing = 0;
    const char *p = name;

    for (; *p == '_'; p++)
        leading++;

    for (; *p; p++)
        trailing = *p == '_' ? trailing + 1 : 0;

    if (leading == 0 && trailing == 0)
        return;

    if (leading == 1 && trailing == 1)
        return;

    error(l, "leading and trailing underscores must be either both absent or both single");
}

static void scan_word(struct lexer *l, struct parser_token *tok, struct parser_pos pos)
{
    static char buf[PARSER_MAX_IDENTIFIER_LENGTH + 1] = {'\0'};
    int bufsize = sizeof(buf)/sizeof(buf[0]);
    char *p = buf;
    int len = 0;
    int alphas = 0;

    int first = get(l);
    if (first == '$' || isword(first)) {
        *p++ = first;
        len++;
    }

    if (isalpha(first))
        alphas++;

    for (int ch = get(l); isword(ch); ch = get(l)) {
        *p++ = ch;
        len++;
        if (len == bufsize) {
            error(l, "too long identifier name. it must have less than %d characters.",
                    bufsize);
        }

        if (isalpha(ch))
            alphas++;
    }
    *p = '\0';

    unget(l);

    if (alphas == 0)
        error(l, "identifier must have at least one alphabet");

    validate_underscores(l, buf);

    int kind = keyword_or_ident(buf);
    tok->sval = data_string_intern(buf);
    set(tok, kind, pos);
}

static void scan_string_literal(struct lexer *l, struct parser_token *tok,
        struct parser_pos pos)
{
    static char buf[PARSER_MAX_STRING_LITERAL_LENGTH + 1] = {'\0'};
    int bufsize = sizeof(buf)/sizeof(buf[0]);
    char *dst = buf;

    struct parser_pos strpos = pos;
    int len = 0;

    for (int ch = get(l); ch != '"'; ch = get(l)) {

        if (ch == '\\') {
            ch = get(l);
            int escseq = parser_find_escape_sequence(ch);
            if (escseq != -1) {
                ch = escseq;
            }
            else {
                error(l, "unknown escape character");
            }
        }
        else if (ch == EOF || ch == '\0') {
            unget(l);
            l->pos = strpos;
            error(l, "unterminated string literal");
        }

        *dst++ = ch;
        len++;

        if (len == bufsize) {
            error(l, "too long string literal. it must have less than %d characters.",
                    bufsize);
        }
    }
    *dst = '\0';

    tok->sval = data_string_intern(buf);
    set(tok, TOK_STRINGLIT, pos);
}

static void scan_line_comment(struct lexer *l)
{
    for (;;) {
        int ch = get(l);

        if (ch == '\n') {
            unget(l);
            break;
        }
    }
}

static void scan_block_comment(struct lexer *l, struct parser_pos pos)
{
    struct parser_pos commentpos = pos;
    /* already accepted "slash-star" */
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

static int count_indent(struct lexer *l)
{
    int indent = 0;

    for (;;) {
        int ch = get(l);

        if (ch == '/') {
            /* indent + line comment => next line */
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
            /* blank line => next line */
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

static int scan_indent(struct lexer *l, struct parser_token *tok)
{
    int indent = count_indent(l);

    if (indent > top(l)) {
        /* push indent */
        push(l, indent);
        set(tok, TOK_BLOCKBEGIN, l->pos);

        /* BlockBegin alwasy starts at beginning of line */
        tok->pos.x = 1;

        return tok->kind;
    }
    else if (indent < top(l)) {
        /* pop indents until it matches current */
        l->unread_blockend = 0;

        while (indent < top(l)) {
            pop(l);

            if (indent == top(l)) {
                set(tok, TOK_BLOCKEND, l->pos);
                return tok->kind;
            }

            l->unread_blockend++;
        }

        /* no indent matches current */
        error(l, "mismatch outer indent");
        return tok->kind;
    }
    else {
        /* no indent change */
        return tok->kind;
    }
}

static void get_token(struct lexer *l, struct parser_token *tok)
{
    static const struct parser_token ini = {0};
    *tok = ini;

    if (l->unread_blockend > 0) {
        l->unread_blockend--;
        set(tok, TOK_BLOCKEND, l->pos);
        return;
    }

    if (l->is_line_begin) {
        l->is_line_begin = false;

        int kind = scan_indent(l, tok);
        if (kind == TOK_BLOCKBEGIN || kind == TOK_BLOCKEND)
            return;
    }

    while (!eof(l)) {
        int ch = get(l);
        struct parser_pos pos = l->pos;

        /* decimal */
        if (isdigit(ch)) {
            unget(l);
            scan_decimal(l, tok, pos);
            return;
        }

        if (ch == '\'') {
            scan_char_literal(l, tok, pos);
            return;
        }

        if (ch == '=') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_EQUAL2, pos);
            }
            else {
                unget(l);
                set(tok, TOK_EQUAL, pos);
            }
            return;
        }

        if (ch == '!') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_EXCLAMEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_EXCLAM, pos);
            }
            return;
        }

        if (ch == '^') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_CARETEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_CARET, pos);
            }
            return;
        }

        if (ch == '~') {
            set(tok, TOK_TILDE, pos);
            return;
        }

        if (ch == '<') {
            ch = get(l);
            if (ch == '<') {
                ch = get(l);
                if (ch == '=') {
                    set(tok, TOK_LT2EQ, pos);
                }
                else {
                    unget(l);
                    set(tok, TOK_LT2, pos);
                }
            }
            else if (ch == '=') {
                set(tok, TOK_LTE, pos);
            }
            else {
                unget(l);
                set(tok, TOK_LT, pos);
            }
            return;
        }

        if (ch == '>') {
            ch = get(l);
            if (ch == '>') {
                ch = get(l);
                if (ch == '=') {
                    set(tok, TOK_GT2EQ, pos);
                }
                else {
                    unget(l);
                    set(tok, TOK_GT2, pos);
                }
            }
            else if (ch == '=') {
                set(tok, TOK_GTE, pos);
            }
            else {
                unget(l);
                set(tok, TOK_GT, pos);
            }
            return;
        }

        if (ch == '+') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_PLUSEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_PLUS, pos);
            }
            return;
        }

        if (ch == '-') {
            ch = get(l);
            if (ch == '-') {
                ch = peek(l);
                if (ch == '-') {
                    get(l);
                    set(tok, TOK_MINUS3, pos);
                }
                else {
                    unget(l);
                    set(tok, TOK_MINUS, pos);
                }
            }
            else if (ch == '=') {
                set(tok, TOK_MINUSEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_MINUS, pos);
            }
            return;
        }

        if (ch == '*') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_ASTEREQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_ASTER, pos);
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
                set(tok, TOK_SLASHEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_SLASH, pos);
            }
            return;
        }

        if (ch == '%') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_PERCENTEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_PERCENT, pos);
            }
            return;
        }

        if (ch == '|') {
            ch = get(l);
            if (ch == '|') {
                set(tok, TOK_VBAR2, pos);
            }
            else if (ch == '=') {
                set(tok, TOK_VBAREQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_VBAR, pos);
            }
            return;
        }

        if (ch == '&') {
            ch = get(l);
            if (ch == '&') {
                set(tok, TOK_AMPERSAND2, pos);
            }
            else if (ch == '=') {
                set(tok, TOK_AMPERSANDEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_AMPERSAND, pos);
            }
            return;
        }

        if (ch == '.') {
            ch = get(l);
            if (ch == '.') {
                set(tok, TOK_PERIOD2, pos);
            }
            else {
                unget(l);
                set(tok, TOK_PERIOD, pos);
            }
            return;
        }

        if (ch == ',') {
            set(tok, TOK_COMMA, pos);
            return;
        }

        if (ch == ';') {
            set(tok, TOK_SEMICOLON, pos);
            return;
        }

        if (ch == ':') {
            ch = get(l);
            if (ch == ':') {
                set(tok, TOK_COLON2, pos);
            }
            else {
                unget(l);
                set(tok, TOK_COLON, pos);
            }
            return;
        }

        if (ch == '(') {
            l->grouping_depth++;
            set(tok, TOK_LPAREN, pos);
            return;
        }

        if (ch == ')') {
            l->grouping_depth--;
            set(tok, TOK_RPAREN, pos);
            return;
        }

        if (ch == '[') {
            l->grouping_depth++;
            set(tok, TOK_LBRACK, pos);
            return;
        }

        if (ch == ']') {
            l->grouping_depth--;
            set(tok, TOK_RBRACK, pos);
            return;
        }

        if (ch == '{') {
            l->grouping_depth++;
            set(tok, TOK_LBRACE, pos);
            return;
        }

        if (ch == '}') {
            l->grouping_depth--;
            set(tok, TOK_RBRACE, pos);
            return;
        }

        /* word */
        if (isalpha(ch) || ch == '_') {
            unget(l);
            scan_word(l, tok, pos);
            return;
        }

        if (ch == '$') {
            unget(l);
            scan_word(l, tok, pos);
            if (tok->kind == TOK_IDENT) {
                l->pos = pos;
                error(l, "unknown special variables: '%s'", tok->sval);
            }
            return;
        }

        /* string */
        if (ch == '"') {
            scan_string_literal(l, tok, pos);
            return;
        }

        if (ch == '#') {
            ch = get(l);
            if (ch == '#') {
                set(tok, TOK_HASH2, pos);
            }
            else {
                unget(l);
                set(tok, TOK_HASH, pos);
            }
            return;
        }

        if (ch == '\n') {
            if (l->grouping_depth > 0)
                continue;

            set(tok, TOK_NEWLINE, pos);
            l->is_line_begin = true;
            return;
        }

        if (ch == EOF) {
            set(tok, TOK_EOF, pos);
            return;
        }

        /* skip */
        if (ch == ' ' || ch == '\t' || ch == '\v') {
            continue;
        }

        error(l, "unknown token");
        return;
    }

    set(tok, TOK_EOF, l->pos);
}

static struct parser_token *new_token(int kind)
{
    struct parser_token *t;

    t = calloc(1, sizeof(*t));
    t-> kind = kind;

    return t;
}

const struct parser_token *parser_tokenize(const char *src, const char *filename)
{
    struct lexer l = {0};
    set_input(&l, src, filename);

    struct parser_token *head = new_token(TOK_ROOT);
    struct parser_token *tail = head;

    while (tail->kind != TOK_EOF) {
        struct parser_token *tok = new_token(0);
        get_token(&l, tok);

        tail->next = tok;
        tok->prev = tail;
        tail = tok;
    }

    return head;
}
