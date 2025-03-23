#ifndef DATA_INTERN_H
#define DATA_INTERN_H

void data_intern_table_init(void);
void data_intern_table_free(void);

const char *data_string_intern(const char *str);
void data_print_intern_table(void);

#endif /* _H */
