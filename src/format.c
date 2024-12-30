#include "format.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

static const char *parse_flags(const char *formats, struct format_spec *spec, char *flags)
{
    static const char orders[] = "-+# 0";
    const char *order = orders;
    const char *fmt = formats;
    char *flag = flags;

    while (true) {
        int ch = *fmt;

        switch (ch) {

        case '-':
            spec->align = FMT_ALIGN_LEFT;
            break;

        case '+':
            spec->show_plus = true;
            break;

        case '#':
            spec->alternate = true;
            break;

        case ' ':
            if (spec->show_plus) {
                spec->errmsg = "show plus '+' and positive number space ' ' cannot be combined";
                return fmt;
            }
            spec->positive_space = true;
            break;

        case '0':
            if (spec->align == FMT_ALIGN_LEFT) {
                spec->errmsg = "align left '-' and zero padding '0' cannot be combined";
                return fmt;
            }
            spec->padding = FMT_PAD_ZERO;
            break;

        default:
            *flag = '\0';
            return fmt;
        }

        order = strchr(order, ch);
        if (!order) {
            spec->errmsg = "invalid flag order: expected [-+ 0]";
            return fmt;
        }

        *flag++ = ch;
        fmt++;
    }

    return NULL;
}

static const char *parse_width(const char *formats, struct format_spec *spec)
{
    const char *fmt = formats;

    if (isdigit(*fmt) && *fmt != '0') {
        char *end = NULL;
        long width = strtol(fmt, &end, 10);
        if (width >= 1024) {
            spec->errmsg = "width must be less than 1024";
            return fmt;
        }
        if (!end) {
            spec->errmsg = "fail to scan width field";
            return fmt;
        }
        spec->width = width;
        return end;
    }

    return fmt;
}

static const char *parse_precision(const char *formats, struct format_spec *spec)
{
    const char *fmt = formats;

    if (*fmt == '.') {
        fmt++;
        char *end = NULL;
        long precision = strtol(fmt, &end, 10);
        if (precision >= 64) {
            spec->errmsg = "precision must be less than 64";
            return fmt;
        }
        if (!end) {
            spec->errmsg = "fail to scan precision field";
            return fmt;
        }
        spec->precision = precision;
        return end;
    }

    return fmt;
}

static const char *parse_type(const char *formats, struct format_spec *spec,
        const char **c_type)
{
    const char *fmt = formats;

    switch (*fmt) {

    case 's':
        if (spec->alternate) {
            spec->errmsg = "flag '#' and type 's' cannot be combined";
            return fmt;
        }
        spec->type = FMT_TYPE_STRING;
        *c_type = "s";
        break;

    case 'd':
        if (spec->alternate) {
            spec->errmsg = "flag '#' and type 'd' cannot be combined";
            return fmt;
        }
        spec->type = FMT_TYPE_DECIMAL;
        *c_type = "lld";
        break;

    case 'c':
        if (spec->alternate) {
            spec->errmsg = "flag '#' and type 'c' cannot be combined";
            return fmt;
        }
        spec->type = FMT_TYPE_CHAR;
        *c_type = "c";
        break;

    case 'o':
        spec->type = FMT_TYPE_OCTAL;
        *c_type = "o";
        break;

    case 'x':
        spec->type = FMT_TYPE_HEX;
        *c_type = "x";
        break;

    case 'X':
        spec->type = FMT_TYPE_HEX;
        *c_type = "X";
        break;

    case 'f':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "f";
        break;

    case 'F':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "F";
        break;

    case 'e':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "e";
        break;

    case 'E':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "E";
        break;

    case 'g':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "g";
        break;

    case 'G':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "G";
        break;

    default:
        spec->errmsg = "invalid type field";
        return fmt;
    }

    return ++fmt;
}

const char *format_parse_specifier(const char *formats, struct format_spec *spec,
        char *c_spec, int c_spec_max_size)
{
    static const struct format_spec default_spec = {0};
    *spec = default_spec;

    /* C format */
    char c_flags[8] = {'\0'};
    const char *c_type = "d";
    const char *fmt = formats;

    if (*fmt != '%') {
        spec->errmsg = "format specifier must start with '%'";
        return fmt;
    }
    fmt++;

    /* percent */
    if (*fmt == '%') {
        spec->type = FMT_TYPE_PERCENT;
        return ++fmt;
    }

    /* flags */
    fmt = parse_flags(fmt, spec, c_flags);
    if (spec->errmsg)
        return fmt;

    /* width */
    fmt = parse_width(fmt, spec);
    if (spec->errmsg)
        return fmt;

    /* precision */
    fmt = parse_precision(fmt, spec);
    if (spec->errmsg)
        return fmt;

    /* type */
    fmt = parse_type(fmt, spec, &c_type);
    if (spec->errmsg)
        return fmt;

    /* c format spec */
    if (c_spec) {
#define BUFSIZE 16
        char c_width[BUFSIZE] = {'\0'};
        char c_precision[BUFSIZE] = {'\0'};

        if (spec->width > 0)
            snprintf(c_width, BUFSIZE, "%d", spec->width);

        if (spec->precision > 0)
            snprintf(c_precision, BUFSIZE, ".%d", spec->precision);

        snprintf(c_spec, c_spec_max_size, "%%%s%s%s%s",
                c_flags, c_width, c_precision, c_type);
#undef BUFSIZE
    }

    return fmt;
}

bool format_is_spec_int(const struct format_spec *spec)
{
    return spec->type == FMT_TYPE_DECIMAL ||
        spec->type == FMT_TYPE_CHAR ||
        spec->type == FMT_TYPE_OCTAL ||
        spec->type == FMT_TYPE_HEX;
}

bool format_is_spec_float(const struct format_spec *spec)
{
    return spec->type == FMT_TYPE_FLOAT;
}

bool format_is_spec_string(const struct format_spec *spec)
{
    return spec->type == FMT_TYPE_STRING;
}

bool format_is_spec_percent(const struct format_spec *spec)
{
    return spec->type == FMT_TYPE_PERCENT;
}
