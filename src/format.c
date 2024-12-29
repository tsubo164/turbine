#include "format.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

const char *format_parse_specifier(const char *formats, struct format_spec *spec)
{
    static const struct format_spec default_spec = {0};
    *spec = default_spec;

    /* C format */
    const char *c_align = "";
    const char *c_pad = "";
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
    if (*fmt == '-') {
        spec->align = FMT_ALIGN_LEFT;
        c_align = "-";
        fmt++;
    }

    /* padding */
    if (*fmt == '0') {
        if (spec->align == FMT_ALIGN_LEFT) {
            spec->errmsg = "align left '-' and zero padding '0' cannot be combined";
            return fmt;
        }
        spec->padding = FMT_PAD_ZERO;
        c_pad = "0";
        fmt++;
    }

    /* width */
    if (isdigit(*fmt) && *fmt != '0') {
        char *end = NULL;
        int width = strtol(fmt, &end, 10);
        if (width >= 1024) {
            spec->errmsg = "width must be less than 1024";
            return fmt;
        }
        spec->width = width;
        fmt = end;
    }
    else {
        spec->width = 1;
    }

    /* type */
    switch (*fmt) {
    case 'd':
        spec->type = FMT_TYPE_DECIMAL;
        c_type = "lld";
        break;

    case 'o':
        spec->type = FMT_TYPE_OCTAL;
        c_type = "o";
        break;

    case 'x':
        spec->type = FMT_TYPE_HEX;
        c_type = "x";
        break;

    case 'X':
        spec->type = FMT_TYPE_HEX;
        c_type = "X"; 
        break;

    case 'f':
        spec->type = FMT_TYPE_FLOAT;
        c_type = "g";
        break;

    case 's':
        spec->type = FMT_TYPE_STRING;
        c_type = "s"; 
        break;

    default:
        spec->errmsg = "invalid type field";
        return fmt;
    }
    fmt++;

    /* c format spec */
    snprintf(spec->cspec, 16, "%%%s%s%d%s", c_align, c_pad, spec->width, c_type);

    return fmt;
}

bool format_is_spec_int(const struct format_spec *spec)
{
    return spec->type == FMT_TYPE_DECIMAL ||
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
