#ifndef FORMAT_H
#define FORMAT_H

#include <stdbool.h>

struct format_spec {
    /* flags */
    char align;
    char plussign;
    char group1k;
    char pad;
    bool alternate;
    bool pointzero;

    int  width;
    int  precision;
    char type;

    const char *errmsg;
};

const char *format_parse_specifier(const char *formats, struct format_spec *spec,
        char *c_spec, int c_spec_max_size);

bool format_is_spec_align_left(const struct format_spec *spec);

bool format_is_spec_bool(const struct format_spec *spec);
bool format_is_spec_int(const struct format_spec *spec);
bool format_is_spec_float(const struct format_spec *spec);
bool format_is_spec_string(const struct format_spec *spec);
bool format_is_spec_percent(const struct format_spec *spec);

#endif /* _H */
