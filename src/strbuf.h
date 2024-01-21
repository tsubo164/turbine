#ifndef STRBUF_H
#define STRBUF_H

typedef struct Strbuf {
    char *data;
    int len;
    int cap;
} Strbuf;

void StrbufCopy(Strbuf *sb, const char *s);
void StrbufCat(Strbuf *sb, const char *s);

#endif // _H
