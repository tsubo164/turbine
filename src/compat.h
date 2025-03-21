#ifndef COMPAT_H
#define COMPAT_H

#if defined(_WIN32) || defined(_WIN64)
  #define OS_WINDOWS
#elif defined(__linux__)
  #define OS_LINUX
#elif defined(__APPLE__) || defined(MACOSX)
  #define OS_MAC
#else
  #define OS_UNIX
#endif

#endif /* _H */
