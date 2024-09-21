#ifndef MEM_H
#define MEM_H

#include <stdlib.h>

#define NALLOC(n,type) calloc((n),sizeof(type))
#define CALLOC(type) NALLOC(1,type)
#define REALLOC(ptr,n) realloc((ptr),(n)*sizeof(*(ptr)))

#endif // _H
