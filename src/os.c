#include "os.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <limits.h>

char *os_get_current_directory(void)
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
    char joined[PATH_MAX] = {'\0'};
    size_t joined_max = sizeof(joined)/sizeof(joined[0]);

    size_t written = snprintf(joined, joined_max, "%s/%s", pathleft, pathright);

    if (written >= joined_max)
        return NULL;

    return realpath(joined, NULL);
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
