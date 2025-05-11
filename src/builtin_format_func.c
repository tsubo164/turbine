#include "builtin_format_func.h"
#include "runtime_string.h"
#include "runtime_value.h"
#include "data_strbuf.h"
#include "format.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

static void format_width(struct data_strbuf *sb, const char *src,
        const struct format_spec *spec, bool is_positive_number)
{
    if (spec->width > 0) {
        int len = strlen(src);
        int width = spec->width;
        int pads = width > len ? width - len : 0;

        if (format_is_spec_align_left(spec)) {
            if (spec->plussign && is_positive_number) {
                data_strbuf_push(sb, spec->plussign);
                pads--;
            }

            data_strbuf_cat(sb, src);
            data_strbuf_pushn(sb, spec->pad, pads);
        }
        else {
            if (spec->plussign && spec->pad == '0' && is_positive_number) {
                data_strbuf_push(sb, spec->plussign);
                pads--;
            }

            if (spec->plussign && spec->pad == ' ' && is_positive_number) {
                data_strbuf_pushn(sb, spec->pad, pads - 1);
                data_strbuf_push(sb, spec->plussign);
            }
            else {
                data_strbuf_pushn(sb, spec->pad, pads);
            }

            data_strbuf_cat(sb, src);
        }
    }
    else {
        data_strbuf_cat(sb, src);
    }
}

static const char *insert_group_separators(const char *input, char *output, char separator)
{
    const char *dot = strchr(input, '.');
    const char *end = dot ? dot : input + strlen(input);
    const char *src = input;
    char *dst = output;

    while (src < end) {
        if ((end - src) % 3 == 0 && isdigit(*src))
            *dst++ = separator;
        *dst++ = *src++;
    }

    while (*src)
        *dst++ = *src++;

    *dst = '\0';

    return output;
}

#define BUFSIZE 64
static void format_int(struct data_strbuf *sb, const struct format_spec *spec,
        const char *c_spec, value_int_t inum)
{
    char buf[BUFSIZE] = {'\0'};
    const char *outputbuf = buf;

    snprintf(buf, BUFSIZE, c_spec, inum);

    if (spec->group1k) {
        char buf1k[BUFSIZE] = {'\0'};
        outputbuf = insert_group_separators(buf, buf1k, spec->group1k);
    }

    format_width(sb, outputbuf, spec, inum > 0);
}

static void format_float(struct data_strbuf *sb, const struct format_spec *spec,
        const char *c_spec, double fpnum)
{
    char buf[BUFSIZE] = {'\0'};
    const char *outputbuf = buf;

    snprintf(buf, BUFSIZE, c_spec, fpnum);

    if (spec->pointzero && fmod(fpnum, 1.0) == 0.0) {
        int len = strlen(buf);
        buf[len]   = '.';
        buf[len+1] = '0';
    }

    if (spec->group1k) {
        char buf1k[BUFSIZE] = {'\0'};
        outputbuf = insert_group_separators(buf, buf1k, spec->group1k);
    }

    format_width(sb, outputbuf, spec, fpnum > 0.0);
}
#undef BUFSIZE

static void format_string(struct data_strbuf *sb, const struct format_spec *spec,
        const char *src)
{
    int len = strlen(src);

    if (spec->precision > 0) {
        if (len > spec->precision)
            len = spec->precision;
    }

    if (spec->width > 0) {
        int width = spec->width;
        int pads = width > len ? width - len : 0;

        if (format_is_spec_align_left(spec)) {
            data_strbuf_catn(sb, src, len);
            data_strbuf_pushn(sb, ' ', pads);
        }
        else {
            data_strbuf_pushn(sb, ' ', pads);
            data_strbuf_catn(sb, src, len);
        }
    }
    else {
        data_strbuf_catn(sb, src, len);
    }
}

void builtin_format_func(const struct runtime_value *args, const char *format,
        struct data_strbuf *result)
{
    struct data_strbuf *sb = result;
    const struct runtime_value *arg = args;
    const char *fmt = format;

    while (*fmt) {

        if (*fmt == '%') {

            struct format_spec spec = {0};
            char c_spec[32] = {'\0'};
            int c_spec_size = sizeof(c_spec)/sizeof(c_spec[0]);

            fmt = format_parse_specifier(fmt, &spec, c_spec, c_spec_size);
            assert(!spec.errmsg);
            arg++;

            if (format_is_spec_bool(&spec)) {
                format_string(sb, &spec, arg->inum ? "true" : "false");
            }
            else if (format_is_spec_int(&spec)) {
                format_int(sb, &spec, c_spec, arg->inum);
            }
            else if(format_is_spec_float(&spec)) {
                format_float(sb, &spec, c_spec, arg->fpnum);
            }
            else if(format_is_spec_string(&spec)) {
                format_string(sb, &spec, runtime_string_get_cstr(arg->string));
            }
            else {
                data_strbuf_push(sb, '%');
                continue;
            }
        }
        else {
            data_strbuf_push(sb, *fmt++);
        }
    }
}
