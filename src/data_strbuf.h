#ifndef DATA_STRBUF_H
#define DATA_STRBUF_H

struct data_strbuf {
    char *data;
    int len;
    int cap;
};

#define DATA_STRBUF_INIT {0}

void data_strbuf_copy(struct data_strbuf *sb, const char *s);
void data_strbuf_cat(struct data_strbuf *sb, const char *s);
void data_strbuf_free(struct data_strbuf *sb);

#endif /* _H */
