#include "read_file.h"
#include "data_strbuf.h"

#include <stdbool.h>
#include <stdio.h>

char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    struct data_strbuf sb = DATA_STRBUF_INIT;
    while (true) {
        int c = fgetc(fp);
        if (c == EOF)
            break;
        data_strbuf_push(&sb, c);
    }
    /* add NL */
    data_strbuf_cat(&sb, "\n");

    fclose(fp);
    return sb.data;
}
