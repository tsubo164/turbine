#ifndef OS_H
#define OS_H

#include <stdbool.h>

char *os_get_current_directory(void);
bool os_path_exists(const char *path);
char *os_path_join(const char *pathleft, const char *pathright);
char *os_dirname(const char *path);

void os_sleep(double seconds);

#endif /* _H */
