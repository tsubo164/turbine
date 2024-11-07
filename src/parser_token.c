#include "parser_token.h"
#include "data_intern.h"
#include "escseq.h"
#include "error.h"
#include "mem.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

const char *parser_get_token_string(int kind)
{
    static const char *table[] = {
    [TOK_NUL]                        =        "nul",
    /* type */
    [TOK_KEYWORD_BEGIN]                        = "keyword_begin",
    [TOK_NIL]                        =        "nil",
    [TOK_TRUE]                        =       "true",
    [TOK_FALSE]                        =      "false",
    [TOK_BOOL]                        =       "bool",
    [TOK_INT]                        =        "int",
    [TOK_FLOAT]                        =      "float",
    [TOK_STRING]                        =     "string",
    /* stmt */
    [TOK_IF]                        =         "if",
    [TOK_FOR]                        =        "for",
    [TOK_ELSE]                        =       "or",
    [TOK_BREAK]                        =      "break",
    [TOK_CONTINUE]                        =   "continue",
    [TOK_SWITCH]                        =     "switch",
    [TOK_CASE]                        =       "case",
    [TOK_DEFAULT]                        =    "default",
    [TOK_RETURN]                        =     "return",
    [TOK_NOP]                        =        "nop",
    [TOK_EXPR]                        =       "expr",
    [TOK_BLOCK]                        =      "block",
    /* special */
    [TOK_CALLER_LINE]                        = "$caller_line",
    [TOK_KEYWORD_END]                        = "keyword_end",
    /* identifier */
    [TOK_FIELD]                        =      "field",
    [TOK_IDENT]                        =      "ident",
    [TOK_FUNC]                        =       "func",
    [TOK_VAR]                        =        "var",
    /* literal */
    [TOK_NILLIT]                        =     "nil_lit",
    [TOK_BOOLLIT]                        =    "bool_lit",
    [TOK_INTLIT]                        =     "int_lit",
    [TOK_FLOATLIT]                        =   "float_lit",
    [TOK_STRINGLIT]                        =  "string_lit",
    [TOK_FUNCLIT]                        =    "func_lit",
    [TOK_ARRAYLIT]                        =   "array_lit",
    [TOK_STRUCTLIT]                        =  "struct_lit",
    /* separator */
    [TOK_LPAREN]                        =     "(",
    [TOK_RPAREN]                        =     ")",
    [TOK_LBRACK]                        =     "[",
    [TOK_RBRACK]                        =     "]",
    [TOK_LBRACE]                        =     "{",
    [TOK_RBRACE]                        =     "}",
    [TOK_SEMICOLON]                        =  ";",
    [TOK_COLON]                        =      ":",
    [TOK_COLON2]                        =     "::",
    [TOK_BLOCKBEGIN]                        = "block_begin",
    [TOK_BLOCKEND]                        =   "block_end",
    [TOK_DASH3]                        =      "---",
    [TOK_DOT]                        =        ".",
    [TOK_COMMA]                        =      ",",
    [TOK_HASH]                        =       "#",
    [TOK_HASH2]                        =      "##",
    [TOK_NEWLINE]                        =    "\\n",
    /* binary */
    [TOK_ADD]                        =        "+",
    [TOK_SUB]                        =        "-",
    [TOK_MUL]                        =        "*",
    [TOK_DIV]                        =        "/",
    [TOK_REM]                        =        "%",
    /* relational */
    [TOK_EQ]                        =         "==",
    [TOK_NEQ]                        =        "!=",
    [TOK_LT]                        =         "<",
    [TOK_LTE]                        =        "<=",
    [TOK_GT]                        =         ">",
    [TOK_GTE]                        =        ">=",
    /* bitwise */
    [TOK_SHL]                        =        "<<",
    [TOK_SHR]                        =        ">>",
    [TOK_OR]                        =         "|",
    [TOK_XOR]                        =        "^",
    [TOK_AND]                        =        "&",
    [TOK_LOR]                        =        "||",
    [TOK_LAND]                        =       "&&",
    /* array, struct, func */
    [TOK_SELECT]                        =     "select",
    [TOK_INDEX]                        =      "index",
    [TOK_CALL]                        =       "call",
    /* unary */
    [TOK_LNOT]                        =       "!",
    [TOK_POS]                        =        "+(pos)",
    [TOK_NEG]                        =        "-(neg)",
    [TOK_ADR]                        =        "&(addr)",
    [TOK_DRF]                        =        "*(deref)",
    [TOK_NOT]                        =        "~",
    [TOK_INC]                        =        "++",
    [TOK_DEC]                        =        "--",
    [TOK_CONV]                        =       "conversion",
    /* assign */
    [TOK_ASSN]                        =       "=",
    [TOK_AADD]                        =       "+=",
    [TOK_ASUB]                        =       "-=",
    [TOK_AMUL]                        =       "*=",
    [TOK_ADIV]                        =       "/=",
    [TOK_AREM]                        =       "%=",
    [TOK_INIT]                        =       "init",
    [TOK_ELEMENT]                        =    "element",
    /* eof */
    [TOK_EOF]                        =        "EOF",
    };

    int count = sizeof(table) / sizeof(table[0]);
    
    if (kind < 0 || kind >= count)
        return NULL;

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

typedef struct Lexer {
    // src text
    const char *src;
    const char *it;
    struct parser_pos pos;
    int prevx;

    // indent
    int indent_stack[128];
    int sp;
    int unread_blockend;
    bool is_line_begin;
    bool is_inside_brackets;
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
    l->prevx = 0;

    l->indent_stack[0] = 0;
    l->sp = 0;
    l->unread_blockend = 0;

    l->is_line_begin = true;
    l->is_inside_brackets = false;
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

static void scan_number(Lexer *l, struct parser_token *tok, struct parser_pos pos)
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
        set(tok, TOK_FLOATLIT, pos);
    }
    else {
        tok->ival = strtol(&(*start), &end, base);
        set(tok, TOK_INTLIT, pos);
    }

    assert(end && (len == (end - &(*start))));
}

static void scan_char_literal(Lexer *l, struct parser_token *tok, struct parser_pos pos)
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

static void scan_word(Lexer *l, struct parser_token *tok, struct parser_pos pos)
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
    tok->sval = data_string_intern(buf);
    set(tok, kind, pos);
}

static void scan_string(Lexer *l, struct parser_token *tok, struct parser_pos pos)
{
    static char buf[4096] = {'\0'};
    char *p = buf;

    const struct parser_pos strpos = pos;
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
    tok->sval = data_string_intern(buf);
    set(tok, TOK_STRINGLIT, pos);
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

static void scan_block_comment(Lexer *l, struct parser_pos pos)
{
    const struct parser_pos commentpos = pos;
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

static int scan_indent(Lexer *l, struct parser_token *tok)
{
    const int indent = count_indent(l);

    if (indent > top(l)) {
        // push indent
        push(l, indent);
        set(tok, TOK_BLOCKBEGIN, l->pos);

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
                set(tok, TOK_BLOCKEND, l->pos);
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

static void get_token(Lexer *l, struct parser_token *tok)
{
    const static struct parser_token ini = {0};
    *tok = ini;

    if (l->unread_blockend > 0) {
        l->unread_blockend--;
        set(tok, TOK_BLOCKEND, l->pos);
        return;
    }

    if (l->is_line_begin) {
        l->is_line_begin = false;

        const int kind = scan_indent(l, tok);
        if (kind == TOK_BLOCKBEGIN || kind == TOK_BLOCKEND)
            return;
    }

    while (!eof(l)) {
        int ch = get(l);
        const struct parser_pos pos = l->pos;

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
                set(tok, TOK_EQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_ASSN, pos);
            }
            return;
        }

        if (ch == '!') {
            ch = get(l);

            if (ch == '=') {
                set(tok, TOK_NEQ, pos);
            }
            else {
                unget(l);
                set(tok, TOK_LNOT, pos);
            }
            return;
        }

        if (ch == '^') {
            set(tok, TOK_XOR, pos);
            return;
        }

        if (ch == '~') {
            set(tok, TOK_NOT, pos);
            return;
        }

        if (ch == '<') {
            ch = get(l);
            if (ch == '<') {
                set(tok, TOK_SHL, pos);
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
                set(tok, TOK_SHR, pos);
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
            if (ch == '+') {
                set(tok, TOK_INC, pos);
            }
            else if (ch == '=') {
                set(tok, TOK_AADD, pos);
            }
            else {
                unget(l);
                set(tok, TOK_ADD, pos);
            }
            return;
        }

        if (ch == '-') {
            ch = get(l);
            if (ch == '-') {
                ch = get(l);
                if (ch == '-') {
                    set(tok, TOK_DASH3, pos);
                }
                else {
                    unget(l);
                    set(tok, TOK_DEC, pos);
                }
            }
            else if (ch == '=') {
                set(tok, TOK_ASUB, pos);
            }
            else {
                unget(l);
                set(tok, TOK_SUB, pos);
            }
            return;
        }

        if (ch == '*') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_AMUL, pos);
            }
            else {
                unget(l);
                set(tok, TOK_MUL, pos);
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
                set(tok, TOK_ADIV, pos);
            }
            else {
                unget(l);
                set(tok, TOK_DIV, pos);
            }
            return;
        }

        if (ch == '%') {
            ch = get(l);
            if (ch == '=') {
                set(tok, TOK_AREM, pos);
            }
            else {
                unget(l);
                set(tok, TOK_REM, pos);
            }
            return;
        }

        if (ch == '|') {
            ch = get(l);
            if (ch == '|') {
                set(tok, TOK_LOR, pos);
            }
            else {
                unget(l);
                set(tok, TOK_OR, pos);
            }
            return;
        }

        if (ch == '&') {
            ch = get(l);
            if (ch == '&') {
                set(tok, TOK_LAND, pos);
            }
            else {
                unget(l);
                set(tok, TOK_AND, pos);
            }
            return;
        }

        if (ch == '.') {
            set(tok, TOK_DOT, pos);
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
            l->is_inside_brackets = true;
            set(tok, TOK_LPAREN, pos);
            return;
        }

        if (ch == ')') {
            l->is_inside_brackets = false;
            set(tok, TOK_RPAREN, pos);
            return;
        }

        if (ch == '[') {
            l->is_inside_brackets = true;
            set(tok, TOK_LBRACK, pos);
            return;
        }

        if (ch == ']') {
            l->is_inside_brackets = false;
            set(tok, TOK_RBRACK, pos);
            return;
        }

        if (ch == '{') {
            l->is_inside_brackets = true;
            set(tok, TOK_LBRACE, pos);
            return;
        }

        if (ch == '}') {
            l->is_inside_brackets = false;
            set(tok, TOK_RBRACE, pos);
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
            if (tok->kind == TOK_IDENT) {
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
                set(tok, TOK_HASH2, pos);
            }
            else {
                unget(l);
                set(tok, TOK_HASH, pos);
            }
            return;
        }

        if (ch == '\n') {
            if (l->is_inside_brackets)
                continue;

            set(tok, TOK_NEWLINE, pos);
            l->is_line_begin = true;
            return;
        }

        if (ch == EOF) {
            set(tok, TOK_EOF, pos);
            return;
        }

        // skip
        if (ch == ' ' || ch == '\t' || ch == '\v') {
            continue;
        }

        error(l, "unknown token");
        return;
    }

    set(tok, TOK_EOF, l->pos);
}

const struct parser_token *parser_tokenize(const char *src)
{
    Lexer l = {0};
    set_input(&l, src);

    struct parser_token *head = CALLOC(struct parser_token);
    struct parser_token *tail = head;

    for (;;) {
        struct parser_token *t = CALLOC(struct parser_token);
        get_token(&l, t);

        tail->next = t;
        t->prev = tail;
        tail = t;

        if (t->kind == TOK_EOF)
            break;
    }

    return head;
}
