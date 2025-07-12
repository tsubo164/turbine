#ifndef CODE_STRUCT_H
#define CODE_STRUCT_H

#include "native_module.h"
#include "value_types.h"
#include "data_strbuf.h"

struct code_struct {
    int id;
    int field_count;
    const char *fullname;
    struct data_strbuf val_types;
    /* TODO is_native */
};

struct code_structvec {
    struct code_struct *data;
    int cap;
    int len;
};

/* push */
int code_push_struct(struct code_structvec *v, const char *fullname, int field_count);

/* lookup */
struct code_struct *code_lookup_struct(struct code_structvec *v, int id);
const struct code_struct *code_lookup_const_struct(const struct code_structvec *v, int id);

/* value type */
void code_struct_push_value_type(struct code_struct *s, int val_type);
int code_struct_get_value_type(const struct code_struct *s, int field_id);
int code_struct_get_field_count(const struct code_struct *s);

void code_structvec_free(struct code_structvec *v);

#endif /* _H */
