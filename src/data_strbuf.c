#include "data_strbuf.h"
#include <string.h>
#include <stdlib.h>

/*
 * Calculates the new allocation size based on the current allocation size
 * and the requested new length.
 *
 * - Minimum allocation: 16 bytes
 * - Growth pattern:
 *     - Doubles the allocation size until reaching 16 KB (16,384 bytes),
 *       i.e., 16 -> 32 -> 64 -> ... -> 16,384.
 *     - Once the allocation size reaches 16 KB, grows in increments of 1,024 bytes
 *       until it meets or exceeds the requested length.
 */
static int grow_cap(int curr_cap, int requested)
{
    int new_cap = curr_cap < 16 ? 16 : curr_cap;

    while (new_cap < requested && new_cap < 1024 * 16)
        new_cap *= 2;
    while (new_cap < requested)
        new_cap += 1024;

    return new_cap;
}

static void resize(struct data_strbuf *sb, int new_len)
{
    if (!sb)
        return;

    if (sb->cap >= new_len + 1) {
        /* no reallocation */
        sb->len = new_len;
        sb->data[sb->len] = '\0';
        return;
    }

    int new_cap = grow_cap(sb->cap, new_len + 1);
    char *new_data = realloc(sb->data, new_cap * sizeof(sb->data[0]));
    if (!new_data)
        return;

    sb->data = new_data;
    sb->cap = new_cap;
    sb->len = new_len;
    sb->data[sb->len] = '\0';
}

void data_strbuf_copy(struct data_strbuf *sb, const char *s)
{
    if (!sb || !s)
        return;

    int len = strlen(s);
    resize(sb, len);
    memcpy(sb->data, s, len + 1);
    sb->len = len;
}

void data_strbuf_cat(struct data_strbuf *sb, const char *s)
{
    if (!sb || !s)
        return;

    int len = strlen(s);
    int old_len = sb->len;
    int new_len = sb->len + len;

    resize(sb, new_len);
    memcpy(sb->data + old_len, s, len + 1);
}

void data_strbuf_free(struct data_strbuf *sb)
{
    if (!sb)
        return;
    free(sb->data);
}
