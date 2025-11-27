#ifndef DATA_INTERN_H
#define DATA_INTERN_H

#include <stdint.h>

struct data_intern_table {
    char **buckets;
    int64_t cap;
    int64_t used;
};

void data_intern_table_init(struct data_intern_table *table);
void data_intern_table_clear(struct data_intern_table *table);

const char *data_string_intern(struct data_intern_table *table, const char *str);
void data_print_intern_table(const struct data_intern_table *table);

#endif /* _H */
