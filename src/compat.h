#ifndef COMPAT_H
#define COMPAT_H

/* Windows */
#if defined(_WIN32) || defined(_WIN64)
  #define OS_WINDOWS
/* Linux */
#elif defined(__linux__)
  #define OS_LINUX
/* macOS */
#elif defined(__APPLE__) || defined(MACOSX)
  #define OS_MAC
/* UNIX */
#else
  #define OS_UNIX
#endif

#endif /* _H */
