#include "format.h"
#include "value_types.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

enum format_align {
    FMT_ALIGN_RIGHT = 0,
    FMT_ALIGN_LEFT,
};

enum format_type {
    FMT_TYPE_DECIMAL = 0,
    FMT_TYPE_BOOL,
    FMT_TYPE_CHAR,
    FMT_TYPE_OCTAL,
    FMT_TYPE_HEX,
    FMT_TYPE_FLOAT,
    FMT_TYPE_STRING,
    FMT_TYPE_PERCENT,
};

static void init_spec(struct format_spec *spec)
{
    spec->align = FMT_ALIGN_RIGHT;
    spec->plussign = '\0';
    spec->group1k = '\0';
    spec->pad = ' ';
    spec->alternate = false;

    spec->width = 0;
    spec->precision = 0;
    spec->type = '\0';

    spec->errmsg = NULL;
}

static const char *parse_flags(const char *formats, struct format_spec *spec, char *flags)
{
#define ORDERS "-+# 0,_"
    static const char orders[] = ORDERS;
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
            spec->plussign = ch;
            break;

        case '#':
            spec->alternate = true;
            *flag++ = ch;
            break;

        case ' ':
            if (spec->plussign == '+') {
                spec->errmsg = "plus sign '+' and plus space ' ' cannot be combined";
                return fmt;
            }
            spec->plussign = ch;
            break;

        case '0':
            if (format_is_spec_align_left(spec)) {
                spec->errmsg = "align left '-' and zero padding '0' cannot be combined";
                return fmt;
            }
            spec->pad = ch;
            break;

        case ',':
            if (spec->plussign == '_') {
                spec->errmsg = "thousand grouping ',' and ' ' cannot be combined";
                return fmt;
            }
            spec->group1k = ch;
            break;

        case '_':
            if (spec->plussign == ',') {
                spec->errmsg = "thousand grouping ',' and ' ' cannot be combined";
                return fmt;
            }
            spec->group1k = ch;
            break;

        default:
            *flag = '\0';
            return fmt;
        }

        order = strchr(order, ch);
        if (!order) {
            spec->errmsg = "invalid flag order: expected ["ORDERS"]";
            return fmt;
        }

        fmt++;
    }

    return NULL;
#undef ORDERS
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

    if (format_is_spec_align_left(spec)) {
        spec->errmsg = "width field is required with align left flag";
        return fmt;
    }

    return fmt;
}

static const char *parse_precision(const char *formats, struct format_spec *spec)
{
    const char *fmt = formats;

    if (*fmt == '.') {
        fmt++;

        if (*fmt == '0') {
            /* precision ".0" ensures ".0" for whole numbers */
            spec->pointzero = true;
            return ++fmt;
        }

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
    bool type_g = false;

    switch (*fmt) {

    case 't':
        if (spec->alternate ||
            spec->plussign ||
            spec->group1k ||
            spec->pad == '0' ||
            spec->pointzero) {
            spec->errmsg = "flags and type 't' cannot be combined except for '-'";
            return fmt;
        }
        spec->type = FMT_TYPE_BOOL;
        *c_type = "s";
        break;

    case 's':
        if (spec->plussign) {
            spec->errmsg = "plus number flag and type 's' cannot be combined";
            return fmt;
        }
        if (spec->group1k) {
            spec->errmsg = "thousand grouping and type 's' cannot be combined";
            return fmt;
        }
        if (spec->pad == '0') {
            spec->errmsg = "pad zero and type 's' cannot be combined";
            return fmt;
        }
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
        *c_type = PRIival;
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
        type_g = true;
        break;

    case 'G':
        spec->type = FMT_TYPE_FLOAT;
        *c_type = "G";
        type_g = true;
        break;

    default:
        spec->errmsg = "invalid type field";
        return fmt;
    }

    if (spec->pointzero && !type_g) {
        spec->errmsg = "'.0' precision is allowed only with 'g' or 'G'";
        return fmt;
    }

    return ++fmt;
}

const char *format_parse_specifier(const char *formats, struct format_spec *spec,
        char *c_spec, int c_spec_max_size)
{
    init_spec(spec);


    /* C format */
    char c_flags[16] = {'\0'};
    const char *c_type = PRIival;
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
        char c_precision[16] = {'\0'};
        if (spec->precision > 0)
            snprintf(c_precision, 16, ".%d", spec->precision);

        snprintf(c_spec, c_spec_max_size, "%%%s%s%s", c_flags, c_precision, c_type);
    }

    return fmt;
}

bool format_is_spec_align_left(const struct format_spec *spec)
{
    return spec->align == FMT_ALIGN_LEFT;
}

bool format_is_spec_bool(const struct format_spec *spec)
{
    return spec->type == FMT_TYPE_BOOL;
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
