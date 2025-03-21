#include "os.h"
#include "compat.h"

#if defined(OS_WINDOWS)
  #include "internal/os_win.c"
#elif defined(OS_LINUX)
  #include "internal/os_linux.c"
#elif defined(OS_MAC)
  #include "internal/os_mac.c"
#else
  #include "internal/os_unix.c"
#endif
