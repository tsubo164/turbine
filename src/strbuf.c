#include "strbuf.h"
#include <string.h>
#include <stdlib.h>

// calculate new alloc size based on the current alloc and
// requested new length.
// the new minimum alloc: 16 bytes
// grows 2x until 16 kb bytes e.g. 16 -> 32 -> ... -> 1024
// adds 1024 bytes until it covers new length
static int grow_cap(int old_cap, int requested)
{
    int new_cap = old_cap < 16 ? 16 : old_cap;

    while (new_cap < requested && new_cap < 1024 * 16)
        new_cap *= 2;
    while (new_cap < requested)
        new_cap += 1024;

    return new_cap;
}

static void resize(Strbuf *sb, int new_len)
{
    if (!sb)
        return;

    if (sb->cap >= new_len + 1) {
        // no reallocation
        sb->len = new_len;
        sb->data[sb->len] = '\0';
        return;
    }

    int new_cap = grow_cap(sb->cap, new_len + 1);
    char *new_data = realloc(sb->data, new_cap);
    if (!new_data)
        return;

    sb->data = new_data;
    sb->cap = new_cap;
    sb->len = new_len;
    sb->data[sb->len] = '\0';
}

void StrbufCopy(Strbuf *sb, const char *s)
{
    if (!sb || !s)
        return;

    int len = strlen(s);
    resize(sb, len);
    memcpy(sb->data, s, len + 1);
    sb->len = len;
}

void StrbufCat(Strbuf *sb, const char *s)
{
    if (!sb || !s)
        return;

    int len = strlen(s);
    int old_len = sb->len;
    int new_len = sb->len + len;

    resize(sb, new_len);
    memcpy(sb->data + old_len, s, len + 1);
}
