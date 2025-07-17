#ifndef RUNTIME_STRING_H
#define RUNTIME_STRING_H

#include "runtime_gc.h"
#include "runtime_value.h"

struct runtime_string {
    struct runtime_object obj;
    char *data;
    int len;
};

/* TODO consider making runtime_const_string_new(const char *s) */
struct runtime_string *runtime_string_new(struct runtime_gc *gc, const char *s);
void runtime_string_free(struct runtime_gc *gc, struct runtime_string *str);

int runtime_string_compare_cstr(const struct runtime_string *str, const char *cstr);
int runtime_string_compare(const struct runtime_string *a, const struct runtime_string *b);

int runtime_string_len(const struct runtime_string *s);

/* TODO consider taking 3 arguments so it doesn't create new string without GC */
struct runtime_string *runtime_string_concat(struct runtime_gc *gc,
        const struct runtime_string *a, const struct runtime_string *b);
const char *runtime_string_get_cstr(const struct runtime_string *s);

#endif /* _H */
