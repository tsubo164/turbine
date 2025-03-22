#ifndef OS_H
#define OS_H

#include <stdbool.h>

char *os_get_current_directory(void);
bool os_path_exists(const char *path);
char *os_path_join(const char *pathleft, const char *pathright);
char *os_dirname(const char *path);

double os_time(void); /* system time */
double os_perf(void); /* monotonic clock */
double os_elapsed(double start); /* for os_time() */
void os_sleep(double seconds);

#endif /* _H */
