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

int code_push_struct(struct code_structvec *v, const char *fullname, int field_count);

struct code_struct *code_lookup_struct(struct code_structvec *v, int id);
const struct code_struct *code_lookup_const_struct(const struct code_structvec *v, int id);

void code_structvec_free(struct code_structvec *v);

#endif /* _H */
