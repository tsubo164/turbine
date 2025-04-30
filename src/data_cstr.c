#include "data_cstr.h"
#include <stdlib.h>
#include <string.h>

char *data_strdup(const char *s)
{
    size_t len = strlen(s);
    char *dup = malloc(len + 1);

    if (dup)
        strcpy(dup, s);

    return dup;
}
