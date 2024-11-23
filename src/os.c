#include "os.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>

char *get_current_directory(void)
{
    return getcwd(NULL, 0);
}

bool os_path_exists(const char *path)
{
    if (access(path, F_OK) == 0)
        return true;
    else
        return false;
}

char *os_path_join(const char *pathleft, const char *pathright)
{
    const char *p1 = pathleft;
    const char *p2 = pathright;
    size_t len1 = strlen(p1);
    size_t len2 = strlen(p2);

    if (p1[len1 - 1] == '/')
        len1--;

    if (p2[len2 - 1] == '/')
        len2--;

    if (p2[0] == '.') {
        p2++;
        len2--;
    }

    if (p2[0] == '/') {
        p2++;
        len2--;
    }

    size_t totallen = len1 + len2 + 1; /* + 1 for '/' */
    char *dst = calloc(totallen + 1, sizeof(char));
    char *p;

    p = stpncpy(dst, p1, len1);
    *p++ = '/';
    p = stpncpy(p, p2, len2);
    assert(totallen == strlen(dst));

    return dst;
}

char *os_dirname(const char *path)
{
    size_t len = 0;
    char *slash;

    slash = strrchr(path, '/');

    if (!slash)
        return NULL;

    if (slash == path)
        len = strlen(path);
    else
        len = slash - path;

    char *dst = calloc(len + 1, sizeof(char));
    strncpy(dst, path, len);
    assert(len == strlen(dst));

    return dst;
}
