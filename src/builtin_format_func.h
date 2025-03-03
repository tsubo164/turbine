#ifndef BUILTIN_FORMAT_FUNC_H
#define BUILTIN_FORMAT_FUNC_H

struct data_strbuf;
struct runtime_value;

void builtin_format_func(const struct runtime_value *args, const char *format,
        struct data_strbuf *result);

#endif /* _H */
