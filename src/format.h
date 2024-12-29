#ifndef FORMAT_H
#define FORMAT_H

#include <stdbool.h>

enum format_align {
    FMT_ALIGN_RIGHT = 0,
    FMT_ALIGN_LEFT,
};

enum format_padding {
    FMT_PAD_SPACE = 0,
    FMT_PAD_ZERO,
};

enum format_type {
    FMT_TYPE_DECIMAL = 0,
    FMT_TYPE_OCTAL,
    FMT_TYPE_HEX,
    FMT_TYPE_FLOAT,
    FMT_TYPE_STRING,
    FMT_TYPE_PERCENT,
};

struct format_spec {
    int align;
    int padding;
    int width;
    int type;

    char cspec[16];
    const char *errmsg;
};

const char *format_parse_specifier(const char *formats, struct format_spec *spec);

bool format_is_spec_int(const struct format_spec *spec);
bool format_is_spec_float(const struct format_spec *spec);
bool format_is_spec_string(const struct format_spec *spec);

#endif /* _H */
